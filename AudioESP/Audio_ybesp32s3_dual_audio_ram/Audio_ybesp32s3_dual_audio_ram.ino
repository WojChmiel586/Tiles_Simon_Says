#include <Arduino.h>
#include <SD.h>
#include <FS.h>
#include <SPI.h>
//For the next two libraies to work, if AudioTools by pschatzmann isn't available in the library
//Will have to manually install them via the termial doing 'git install https://github.com/pschatzmann/arduino-audio-tools' into the Arduino Library Folder.
#include "AudioTools.h"
#include "AudioTools/AudioCodecs/CodecWAV.h"
#include <WiFi.h>
#include <esp_now.h>
#include "audio_data.h"

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

#define AUDIO_BUFFER_SIZE 1024

//Defining Varaibles and Functions for Audio Mixer START

//Audio Objects
I2SStream i2s;
InputMixer<int16_t> mixer;
StreamCopy copier(i2s,mixer,AUDIO_BUFFER_SIZE);
AudioInfo info(44100,2,16);

//Background Music
File bgFile;
WAVDecoder bgDecoder;
EncodedAudioStream bgStream(&bgFile,&bgDecoder);
VolumeStream bgVolume(bgStream);

int bgIndex = -1;
const float bgVol = 50;
String currentPath;

//Sound Effects (RAM-Based) Pointers so they can be changed
MemoryStream* sfxMemoryStream;
WAVDecoder* sfxDecoder;
EncodedAudioStream* sfxStream;

int sfxIndex = -1;
const float sfxVol = 100;
bool isSfxPlaying = false;

//Sound Effect start and end times (Milliseconds).
static uint32_t sfxStartTime = 0;
uint32_t sfxDurationMs = 2000;

//Defining Varaibles and Functions for Audio Mixer END

struct AudioFile {
  String name;      // Base name without extension
  String wavPath;   // Full path to WAV file (if exists)
  //String mp3Path;   // Full path to MP3 file (if exists)
  bool hasWav;
  //bool hasMp3;
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

  //Sets up mixer and I2S.   
  audioSetup();
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

  // Set up background music, background music comes from the SD Card because they're larger files, Have to be WAV.
  bgFile = SD.open("/theme.wav");
  currentPath = "/theme.wav";
  if(!bgFile)
  {
    Serial.println("Background music missing.");
    return;   
  }
  bgStream.begin();
  bgVolume.setVolume(0.1);
  bgIndex = mixer.add(bgVolume, 50);
  Serial.println("Background Music Start!");
  
}

void loop() {
  //Keeps audio running
  copier.copy();

  //Detect End of Background Music
  if(!bgFile.available() || bgFile.available() < 2048)
  {
    Serial.println("BG music ended");
    playBg(-1,1); //Not passing in new audio (-1), confirmed its looping instead (1)
  }

  //Detect End of SFX
  if(isSfxPlaying && millis() - sfxStartTime > sfxDurationMs)
  {
    mixer.setWeight(sfxIndex,0);
    //mixer.setWeight(bgIndex,50);
    isSfxPlaying = false;
    Serial.println("Sound Effect End");
  }

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

void audioSetup() {
  // Configure I2S Settings
  auto i2s_config = i2s.defaultConfig(TX_MODE);
  i2s_config.pin_bck = I2S_BCLK;
  i2s_config.pin_ws = I2S_LRC;
  i2s_config.pin_data = I2S_DOUT;
  i2s_config.sample_rate = 44100;    
  i2s_config.bits_per_sample = 16;
  i2s_config.channels = 2;
  i2s_config.buffer_size = AUDIO_BUFFER_SIZE;
  i2s_config.buffer_count = 10; 
  i2s.begin(i2s_config);

  mixer.begin(info);
  //Sfx Stream, allocating base pointers to avoid errors. 
  sfxMemoryStream = new MemoryStream((uint8_t*)sfxFailData,sfxFailLen,true,FLASH_RAM);
  sfxDecoder = new WAVDecoder();
  sfxStream = new EncodedAudioStream(sfxMemoryStream,sfxDecoder);

  sfxStream->begin();
  sfxDecoder->begin();
  sfxIndex = mixer.add(*sfxStream, 0); //Starts Muted
}

void playSfx(int sfx_playing)
{
  
  //Ensures stream is turned off
  mixer.setWeight(sfxIndex,0);
  delay(1);
  //Resets Variables for reasignment
  const uint8_t* currentData = nullptr;
  size_t currentLen = 0;
  delete sfxStream;
  delete sfxDecoder;
  delete sfxMemoryStream;
  // mixer.remove(sfxIndex);

  currentData = sfxList[sfx_playing].data;
  currentLen = sfxList[sfx_playing].len;
  sfxDurationMs = sfxList[sfx_playing].ms;
  /*
  //Changes What sound effect is playing.
    if(sfx_playing == 0)
    {
      currentData = sfxFailData; //Data to change it to
      currentLen = sfxFailLen; // Length of data
      Serial.println("Playing Fail"); 
      sfxDurationMs = 1800; //Length of audio in Milliseconds (Needs to at least equal or less to length to avoid clack noise + distortion)
    }
    else if(sfx_playing == 1)
    {
      currentData = sfxSuccessData;
      currentLen = sfxSuccessLen;
      Serial.println("Playing Success");
      sfxDurationMs = 1020;
    }
    else
    {
      currentData = sfxPartialData;
      currentLen = sfxPartialLen;
      Serial.println("Playing Parital");
      sfxDurationMs = 1500;
    }
  */
  //Serial.printf("Data=%p Len=%u\n", currentData, (unsigned)currentLen);
  sfxMemoryStream = new MemoryStream((uint8_t*)currentData,currentLen,true,FLASH_RAM);
  sfxDecoder = new WAVDecoder();
  sfxStream = new EncodedAudioStream(sfxMemoryStream,sfxDecoder);
  
  //Reinitalise stream into mixer.
  sfxStream->begin();
  sfxDecoder->begin();
  sfxIndex = mixer.add(*sfxStream,0);

  sfxStartTime = millis();
  mixer.setWeight(sfxIndex,100);

  isSfxPlaying = true;
}

void playBg(int bg_playing, int looping)
{
  String pathToPlay;
  if(looping)
  {
    pathToPlay = currentPath;
  }
  else
  {
    if(bg_playing < 0 || bg_playing >= totalAudioFiles) {return;}

    
    if(audioFiles[bg_playing].hasWav)
    {
      pathToPlay = audioFiles[bg_playing].wavPath;
      currentPath = pathToPlay;
    }
    else
    {
      Serial.println("Error: No Valid Audio File Found.");
      return; 
    }
  }
  
  mixer.setWeight(bgIndex,0);
  File newFile = SD.open(pathToPlay.c_str());
  if(!newFile){Serial.println("Missing Bg Music");}

  bgFile.close();
  bgFile = newFile;

  bgDecoder.begin();

  bgStream.end();
  bgStream = EncodedAudioStream(&bgFile, &bgDecoder);
  bgStream.begin();

  mixer.setWeight(bgIndex,100);
  Serial.println("Background Music Starting");
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

  bool isNumber = true;
  for (unsigned int i = 0; i < command.length(); i++)
  {
    if(!isDigit(command.charAt(i)))
    {
      isNumber = false;
      break;
    }
  }
  int fileIndex = -1;
  //For testing purposes, a number is a sound effect, words is the background music
  if(isNumber)
  {
    fileIndex = command.toInt();
    if(fileIndex > sfxAmount - 1) 
    {
      Serial.println("No SFX fit that number");
      return; 
    }
    playSfx(command.toInt());
  }
  else
  {
    for (int i = 0; i < totalAudioFiles; i++)
    {
      if(audioFiles[i].name.equalsIgnoreCase(command))
      {
        fileIndex = i;
        break;
      }
    }
    if(fileIndex < 0)
    {
      Serial.printf("File '%s' not found. Type 'list' to see all files. \n", command.c_str());
      return;
    }
    playBg(fileIndex,0);
  }
  
  return;
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
    
    if (!file.isDirectory() && (fileName.endsWith(".wav") || fileName.endsWith(".WAV")))
    {
      
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
      
      if (existingIndex >= 0) 
      {
        // Add to existing entry
        if (isWav) 
        {
          audioFiles[existingIndex].hasWav = true;
          audioFiles[existingIndex].wavPath = fullPath;
        }
      } 
      else 
      {
        // Create new entry
        if (totalAudioFiles < 100) 
        {
          audioFiles[totalAudioFiles].name = baseName;
          if (isWav) 
          {
            audioFiles[totalAudioFiles].hasWav = true;
            audioFiles[totalAudioFiles].wavPath = fullPath;
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
    Serial.print(" (WAV)");
    Serial.println();
  }
  Serial.println("\n===== AUDIO FILES ON RAM =====");
  Serial.printf("Total files: %d\n\n", sfxAmount);
  for (int j = 0; j < sfxAmount; j++)
  {
    Serial.printf("[%d] %s",j,sfxList[j].name);
    Serial.println();
  }
  Serial.println("\n===================================");
  Serial.println("Type a number to play from RAM and a name to play from SD:");
  Serial.println("Type 'list' to show all files again\n");
}

