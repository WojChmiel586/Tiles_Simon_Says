#include <Arduino.h>
#include <SD.h>
#include <FS.h>
#include <SPI.h>
#include "AudioTools.h"
#include "AudioTools/AudioCodecs/CodecWAV.h"
#include <WiFi.h>
#include <esp_now.h>

// SD Card pins for YB-ESP32-S3-AMP
#define SD_CS 10
#define SD_MOSI 11
#define SD_MISO 13
#define SD_SCK 12

// I2S pins for YB-ESP32-S3-AMP (connected to MAX98357A amplifiers)
#define I2S_BCLK 5   // Bit clock
#define I2S_LRC 6    // Frame clock (LRCLK)
#define I2S_DOUT 7   // Digital audio signal (DIN)

// Status LED
#define STATUS_LED 47


//Defining Varaibles and Functions for Audio Mixer START

//Audio Objects
I2SStream i2s;
InputMixer<int16_t> mixer;
StreamCopy copier(i2s,mixer);
AudioInfo info(4410,2,16);
// Volume Percentages, must add to 1.
const float bgVol = 0.3;
const float sfxVol = 0.7;

//BG and Event Sfx Streams
File bgFile;
File sfxFile;
WAVDecoder bgDecoder;
WAVDecoder sfxDecoder;
EncodedAudioStream bgStream(&bgFile, &bgDecoder);
EncodedAudioStream sfxStream(&sfxFile, &sfxDecoder);
VolumeStream bgVS(bgStream);
VolumeStream sfxVS(sfxStream);
StreamCopy bgCopy(bgStream,bgFile);
StreamCopy sfxCopy(sfxStream,sfxFile);



bool isSfxPlaying = false;

//ESP-NOW Trigger Flags
volatile bool espNowTriggerReceived = false;
//ESP-NOW Structural Format
typedef struct struct_message
{
  char command[32];  
} struct_message; 

struct_message incomingData;

//Updated ESP-NOW Interrupt Callback Function for the modern ESP32 cores
void OnDataRecv(const esp_now_recv_info_t * recv_info, const uint8_t * incomingDataRaw, int len)
{
  memcpy(&incomingData,incomingDataRaw,sizeof(incomingData));
  if (strcmp(incomingData.command, "play_sound") == 0 )
  {
    espNowTriggerReceived = true;    
  }
}

//Defining Varaibles and Functions for Audio Mixer END

struct AudioFile {
  String name;      // Base name without extension
  String wavPath;   // Full path to WAV file (if exists)
  String mp3Path;   // Full path to MP3 file (if exists)
  bool hasWav;
  bool hasMp3;
};

AudioFile audioFiles[100];
int totalAudioFiles = 0;
bool isPlaying = false;
String serialBuffer = "";

// Initalisation
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Setup status LED
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, LOW);
  
  Serial.println("YB-ESP32-S3-AMP Rev3 Audio Player");
  
  // Initialize SD card using SPI
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI);
  
  if (!SD.begin(SD_CS)) {
    Serial.println("SD Card initialization failed!");
    return;
  }
  Serial.println("SD Card initialized");
  
  WiFi.mode(WIFI_STA);
  // Initialised ESP-NOW
  if (esp_now_init() != ESP_OK)
  {
    Serial.println(("Error initalising ESP-NOW"));
    return;    
  } 
  esp_now_register_recv_cb(OnDataRecv);
  Serial.println(("ESP-NOW Ready."));
  
  // Configure I2S Settings
  auto i2s_config = i2s.defaultConfig(TX_MODE);
  i2s_config.pin_bck = I2S_BCLK;
  i2s_config.pin_ws = I2S_LRC;
  i2s_config.pin_data = I2S_DOUT;
  i2s_config.sample_rate = 44100;    
  i2s_config.bits_per_sample = 16;
  i2s_config.channels = 2;
  Serial.println("Starting I2S...");
  bool ok = i2s.begin(i2s_config);

  Serial.printf("I2S begin: %s\n", ok ? "OK" : "FAILED");
  Serial.println("Ending I2S...");

  // Initialising Audio Mixer
  //AudioInfo mixer_config(44100,2,16);
  mixer.begin(info);
  //mixer.add(bgVS);
  mixer.add(sfxVS);
    
  // Scan SD card for audio files
  scanAudioFiles();
  
  if (totalAudioFiles == 0) {
    Serial.println("No audio files found on SD card!");
    return;
  }
  
  // Display all files
  listAllFiles();

  // Blink LED to indicate ready
  for (int i = 0; i < 3; i++) {
    digitalWrite(STATUS_LED, HIGH);
    delay(200);
    digitalWrite(STATUS_LED, LOW);
    delay(200);
  }

  // bgFile = SD.open("/Melody.wav");
  // bgStream.begin();
  // bgVS.setVolume(bgVol);

}

void loop() {
  //Keeps audio streaming
  // bgCopy.copy();
  // sfxCopy.copy();
  copier.copy();

  //Loops Background Music
  // if(bgFile && !bgFile.available())
  // {
  //   bgFile.seek(0);
  //   bgStream.end();
  //   bgStream.begin();
  // }

  //Cleans for completed sfx sounds
  if (isSfxPlaying && !sfxFile.available())
  {
    Serial.println("Event Sound Near end. Fading Out");
    sfxFile.close();
    sfxVS.setVolume(0.0);
    isSfxPlaying = false;
    Serial.println("Sfx Sound Finished Cleanly");
  }

  //Acting on ESP-NOW flags
  if(espNowTriggerReceived)
  {
    espNowTriggerReceived = false;
    playSfxSound();
  }


  /*
  // Check if current file finished playing for "Audio.h"
  if (isPlaying && !audio.isRunning()) {
    //delay(200) //Hopefully to help with clack
    isPlaying = false;
    digitalWrite(STATUS_LED, LOW);
    Serial.println("Playback complete");
    Serial.println("\nType a file number or name to play:");
  }*/

  
  // Read serial input
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (serialBuffer.length() > 0) {
        processSerialCommand(serialBuffer);
        Serial.println("Process Inputted Succesfully");
        serialBuffer = "";
      }
    } else {
      serialBuffer += c;
    }
  }

}

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
    
    if (!file.isDirectory() && 
        (fileName.endsWith(".wav") || fileName.endsWith(".WAV") || 
         fileName.endsWith(".mp3") || fileName.endsWith(".MP3"))) {
      
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
  //Serial.println("Start of Process ----------------------"); //Debugging
  if (command.length() == 0) return;
  
  

  // Check if it's the list command
  if (command.equalsIgnoreCase("list")) {
    listAllFiles();
    return;
  }

  playSfxSound();
  return;
  /*
  // Check if it's a number
  bool isNumber = true;
  for (unsigned int i = 0; i < command.length(); i++) { //Checks each character of the command
    if (!isDigit(command.charAt(i))) { //If the command character is not a digit
      isNumber = false;
      break;
    }
  }
  
  int fileIndex = -1;
  
  if (isNumber) {
    // It's a number, try to parse it
    fileIndex = command.toInt();
    if (fileIndex < 0 || fileIndex >= totalAudioFiles) {
      Serial.printf("Invalid file number. Please enter 0-%d\n", totalAudioFiles - 1);
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
  
  // Play the file
  playAudioFile(fileIndex); //For testing put it above checks to see if that removes delay
  */
}

//for "AudioTools.h" (Mixer)
void playSfxSound()
{
  sfxFile = SD.open("/fail.wav");
  if(sfxFile)
  {
    sfxStream.begin();
    sfxVS.setVolume(sfxVol);
    isSfxPlaying = true;   
    Serial.println("SFX TRiggered");
  }
}