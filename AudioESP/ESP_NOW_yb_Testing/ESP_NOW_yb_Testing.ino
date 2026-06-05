// yb CODE SENDING AND RECEIVING
#include <Arduino.h>
#include <SD.h>
#include <FS.h>
#include <SPI.h>
#include "Audio.h"

// SD Card pins for YB-ESP32-S3-AMP
#define SD_CS 10
#define SD_MOSI 11
#define SD_MISO 13
#define SD_SCK 12

// I2S pins for YB-ESP32-S3-AMP (connected to MAX98357A amplifiers)
#define I2S_BCLK 5  // Bit clock
#define I2S_LRC 6   // Frame clock (LRCLK)
#define I2S_DOUT 7  // Digital audio signal (DIN)

// Status LED
#define STATUS_LED 47

Audio audio;

struct AudioFile {
  String name;     // Base name without extension
  String wavPath;  // Full path to WAV file (if exists)
  String mp3Path;  // Full path to MP3 file (if exists)
  bool hasWav;
  bool hasMp3;
};

AudioFile audioFiles[100];
int totalAudioFiles = 0;
bool isPlaying = false;
String serialBuffer = "";

#include <esp_now.h>  //===============================================ESP
#include <WiFi.h>

// 1. THE MAC ADDRESS OF THE 1Tile LED ESP BOARD (=receiver). we send to this address.
uint8_t receiverAddress[] = { 0xEC, 0xDA, 0x3B, 0x95, 0xC5, 0xC8 };
//ec:da:3b:95:c5:c8

// uniform structure for all data messages
typedef struct struct_message_all {  // sender/receiver must match structure
  int id;                            // unique sender ID: yellobyte ESP = 1, game ESP = 2
  int t;                             // can be used for airtime
  int b;                             // can be used for buttonInput
  int jc;                            // can be used for jumpCount
  int js;                            // can be used for jumpState
  int sd;                            // can be used for stepDelay
  int dA;                            // left toe sensor
  int dB;                            // left heel sensor
  int eA;                            // right toe sensor
  int eB;                            // right heel sensor
  int fA;
  int fB;
  int gA;
  int gB;
} struct_message_all;
struct_message_all myGame;     // Create an outgoing struct_message from yellobyte ESP called myGame
struct_message_all myResults;  // Create an incoming struct_message called myResults

//variables incoming and outgoing
bool dataReceived = false;
int airtime = 550;    //default if no data coming in
int buttonInput = 5;  // default, outgoing from yellobyte ESP = 0, 91, 92, 93, 94, 95, 96, 97, 98
int jumpState;    
int jumpCount;
int stepDelay = 500;  // smallest timing unit. 666 would be equivalent to taking 3 steps in 2 seconds at 90bpm
int leftToe;
int leftHeel;
int rightToe;
int rightHeel;
int gameSuccess = 5;

// --- CALLBACK: DATA SENT ---
void OnDataSent(const esp_now_send_info_t *info, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// --- CALLBACK: DATA RECEIVED ---
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  //const uint8_t *mac_addr = info->src_addr;// Extract MAC address of sender
  memcpy(&myResults, incomingData, sizeof(myResults));
  // Serial.print("\r\nBytes received: ");
  // Serial.println(len);
  // Serial.print("From MAC: ");
  // for (int i = 0; i < 6; i++) {
  //   Serial.printf("%02X%s", mac_addr[i], (i < 5) ? ":" : "");
  // }
  // Serial.printf("\nMessage: %s | Value: %d\n", myResults.msg, myResults.value);
  leftToe = myResults.dA; // fill myResults struct with data
  leftHeel = myResults.dB;
  rightToe = myResults.eA;
  rightHeel = myResults.eB;
  gameSuccess = myResults.gB;
  dataReceived = true;
}

//=========================================================================================
void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register Callbacks using explicit casting to prevent v3.0 errors
  esp_now_register_send_cb((esp_now_send_cb_t)OnDataSent);
  esp_now_register_recv_cb((esp_now_recv_cb_t)OnDataRecv);

  // Register Peer
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  // Add peer to network
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  // Setup status LED for YB board --------------- start Audio
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, LOW);
  Serial.println("YB-ESP32-S3-AMP Rev2 Audio Player");

  // Initialize SD card using SPI
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI);
  SD.begin(SD_CS);
  if (!SD.begin(SD_CS)) {
    Serial.println("SD Card initialization failed!");
    return;
  }
  Serial.println("SD Card initialized");

  // Initialize audio
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(18);  // 0...21, adjust as needed

  // Scan SD card for audio files
  scanAudioFiles();

  if (totalAudioFiles == 0) {
    Serial.println("No audio files found on SD card!");
    return;
  }

  // Display all files
  listAllFiles();

  // Blink LED to indicate ready - why are we checking 0,1,2?
  for (int i = 0; i < 3; i++) {
    digitalWrite(STATUS_LED, HIGH);
    delay(200);
    digitalWrite(STATUS_LED, LOW);
    //delay(200);
  }
}  // end of void setup

//=====================================================================================
void loop() {
  //check for messages from game esp (not keyboard) - for game success sounds
  if (dataReceived == true) {
    loadData();  // load gameSuccess data from game esp, sets dataReceived to false again
    Serial.print("gameSuccess is ");
    Serial.println(gameSuccess);
    /*Serial.print(", LT: ");
    Serial.print(leftToe);  //print the number
    Serial.print(", LH: ");
    Serial.print(leftHeel);  //print the number
    Serial.print(", RT: ");
    Serial.print(rightToe);  //print the number
    Serial.print(", RH: ");
    Serial.println(rightHeel);  //print the number*/
  }

  switch (gameSuccess) {
    case 0:
      playAudioFile(0); //play sound 0 = success 
      gameSuccess = 5;
    break;
    case 1:
      playAudioFile(1); //play sound 1 = partial 
      gameSuccess = 5;
    break;
    case 2:
      playAudioFile(2); //play sound 2 = fail 
      gameSuccess = 5;
    break;
    case 3:
      playAudioFile(3); //play sound 2 = fail 
      gameSuccess = 5;
    break;
    case 5: //idle state
    break;
  } //end of switch

  audio.loop();
  // Check if current file finished playing
  if (isPlaying && !audio.isRunning()) {
    isPlaying = false;
    digitalWrite(STATUS_LED, LOW);
    Serial.println("Playback complete");
    Serial.println("\nType a file number or name to play:");
  }

  // Read serial input -> send number on to game ESP to play a game. once done, we play sound.
  // we need to input 91, 92, 93, 95, 97, or 98 to start a game. Do not send 0 to reset
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (serialBuffer.length() > 0) {
        processSerialCommand(serialBuffer);// this is where we process the number
        serialBuffer = "";
      }
    } else {
      serialBuffer += c;
    }
  }
}  // end of loop 

//==============================================================================

void loadData() {  //actively load incoming results and sensor values, sets dataReceived = false
  //airtime = myResults.t; 
  //jumpState = myResults.js; 
  //stepDelay is 500ms by default
  //stepDelay = myResults.sd; // update stepDelay-if we wanted to alter this.
  leftToe = myResults.dA;
  leftHeel = myResults.dB;
  rightToe = myResults.eA;
  rightHeel = myResults.eB;
  gameSuccess = myResults.gB; 
  dataReceived = false;
}  //end of loadData

void scanAudioFiles() {
  File root = SD.open("/");
  if (!root) {
    Serial.println("Failed to open root directory");
    return;
  }

  totalAudioFiles = 0;
  File file = root.openNextFile();

  while (file) {
    String fileName = String(file.name());

    if (!file.isDirectory() && (fileName.endsWith(".wav") || fileName.endsWith(".WAV") || fileName.endsWith(".mp3") || fileName.endsWith(".MP3"))) {

      // Get base name without extension
      String baseName = fileName;
      int lastDot = baseName.lastIndexOf('.');
      if (lastDot > 0) {
        baseName = baseName.substring(0, lastDot);
      }

      // Check if we already have this base name
      int existingIndex = -1;
      for (int i = 0; i < totalAudioFiles; i++) {
        if (audioFiles[i].name.equalsIgnoreCase(baseName)) {
          existingIndex = i;
          break;
        }
      }

      // Determine if it's WAV or MP3
      bool isWav = fileName.endsWith(".wav") || fileName.endsWith(".WAV");
      String fullPath = "/" + fileName;

      if (existingIndex >= 0) {
        // Add to existing entry
        if (isWav) {
          audioFiles[existingIndex].hasWav = true;
          audioFiles[existingIndex].wavPath = fullPath;
        } else {
          audioFiles[existingIndex].hasMp3 = true;
          audioFiles[existingIndex].mp3Path = fullPath;
        }
      } else {
        // Create new entry
        if (totalAudioFiles < 100) {
          audioFiles[totalAudioFiles].name = baseName;
          if (isWav) {
            audioFiles[totalAudioFiles].hasWav = true;
            audioFiles[totalAudioFiles].wavPath = fullPath;
            audioFiles[totalAudioFiles].hasMp3 = false;
          } else {
            audioFiles[totalAudioFiles].hasMp3 = true;
            audioFiles[totalAudioFiles].mp3Path = fullPath;
            audioFiles[totalAudioFiles].hasWav = false;
          }
          totalAudioFiles++;
        }
      }
    }

    file.close();
    file = root.openNextFile();
  }

  root.close();
}

void listAllFiles() {
  Serial.println("\n===== AUDIO FILES ON SD CARD =====");
  Serial.printf("Total files: %d\n\n", totalAudioFiles);

  for (int i = 0; i < totalAudioFiles; i++) {
    Serial.printf("[%d] %s", i, audioFiles[i].name.c_str());

    if (audioFiles[i].hasWav && audioFiles[i].hasMp3) {
      Serial.print(" (WAV + MP3, will play WAV)");
    } else if (audioFiles[i].hasWav) {
      Serial.print(" (WAV)");
    } else if (audioFiles[i].hasMp3) {
      Serial.print(" (MP3)");
    }
    Serial.println();
  }

  Serial.println("\n===================================");
  Serial.println("Type a file number or name to play:");
  Serial.println("Type 'list' to show all files again");
}

void processSerialCommand(String command) {
  command.trim();

  if (command.length() == 0) return;

  // Check if it's the list command
  if (command.equalsIgnoreCase("list")) {
    listAllFiles();
    return;
  }

  // Check if it's a number
  bool isNumber = true;
  for (unsigned int i = 0; i < command.length(); i++) {
    if (!isDigit(command.charAt(i))) {
      isNumber = false;
      break;
    }
  }

  int fileIndex = -1;  //variable to hold the data entered via serial monitor. cant use 0 as this is assigned

  if (isNumber) {
    // It's a number, try to parse it
    fileIndex = command.toInt();  // set the fileIndex to the int number identified?

    Serial.print("Number entered is: ");
    Serial.println(fileIndex);  //print the number
    //Send a message with the number, even if it is out of range
    myGame.b = fileIndex;  // send this number
    esp_now_send(receiverAddress, (uint8_t *)&myGame, sizeof(myGame));
    //delay(500);

    if (fileIndex < 0 || fileIndex >= totalAudioFiles) {  // these are not within range: 91, 92, 93, 94, 95, 96, 97, 98
      //Serial.printf("Invalid file number. Please enter 0-%d\n", totalAudioFiles - 1); // do nothing
      return;
    }

  } else {
    // It's a name, search for it
    for (int i = 0; i < totalAudioFiles; i++) {
      if (audioFiles[i].name.equalsIgnoreCase(command)) {
        fileIndex = i;
        break;
      }
    }

    if (fileIndex < 0) {
      Serial.printf("File '%s' not found. Type 'list' to see all files.\n", command.c_str());
      return;
    }
  }

  // Go and play the file 
  playAudioFile(fileIndex);
}//end of process Input

void playAudioFile(int index) {
  if (index < 0 || index >= totalAudioFiles) return;// should do nothing if number is outside range

  // Prefer WAV over MP3 if both exist
  String pathToPlay;
  if (audioFiles[index].hasWav) {
    pathToPlay = audioFiles[index].wavPath;
    Serial.printf("Playing WAV: %s\n", audioFiles[index].name.c_str());
  } else if (audioFiles[index].hasMp3) {
    pathToPlay = audioFiles[index].mp3Path;
    Serial.printf("Playing MP3: %s\n", audioFiles[index].name.c_str());
  } else {
    Serial.println("Error: No valid audio file found");
    return;
  }

  digitalWrite(STATUS_LED, HIGH);
  audio.connecttoFS(SD, pathToPlay.c_str());
  isPlaying = true;
}

// Optional: Audio event callbacks for debugging
void audio_info(const char *info) {
  Serial.print("Info: ");
  Serial.println(info);
}

void audio_eof_mp3(const char *info) {
  Serial.print("End of file: ");
  Serial.println(info);
}
