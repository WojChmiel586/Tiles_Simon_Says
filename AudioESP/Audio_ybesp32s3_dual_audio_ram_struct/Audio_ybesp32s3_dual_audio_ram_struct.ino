#include <Arduino.h>
#include <SD.h>
#include <FS.h>
#include <SPI.h>
#include "audio_data.h"
#include "AudioStructs.h"
//For the next libraies to work, if AudioTools by pschatzmann isn't available in the library
//Will have to manually install them via the termial doing 'git install https://github.com/pschatzmann/arduino-audio-tools' into the Arduino Library Folder.
#include "AudioTools.h"
#include "AudioTools/AudioCodecs/CodecWAV.h"
#include "AudioTools/CoreAudio/ResampleStream.h"
//For writing to PSRAM
#include "esp_heap_caps.h"

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
const float bgVol = 1.0;
String currentPath;

SfxVoice sfx_voices[2]; //Amount of Sound effcts that could play at once.
int totalSfxVoices = 2;
//Acting as pointers
int currentSfx = 0;

//Sound Effects (Flash Memory-Based) Pointers so they can be changed
MemoryStream* sfxMemoryStream;
WAVDecoder* sfxDecoder;
EncodedAudioStream* sfxStream;
VolumeStream* sfxVolume;
ResampleStream* sfxResample;

int sfxIndex = -1;
bool isSfxPlaying = false;
float sfxPitch = 1.0f; //Normal.
float sfxVolValue = 0.70;

//Sound Effect start and end times (Milliseconds).
static uint32_t sfxStartTime = 0;
uint32_t sfxDurationMs = 2000;

//Sound Effects (PSRAM)
int numSounds = 7;
SoundEffect sounds[7];
//Debugging Menu
String menu = "home";

enum GameStates {MENU,JUMP,SIMON,CALIBRATION};
GameStates currentGS = MENU;

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

// ----  ESP  ----
#include <WiFi.h>
#include <esp_now.h>
#include "ESPEstruct.h"

//Going to Game ESP
uint8_t receiverAddress[] = { 0xEC, 0xDA, 0x3B, 0x95, 0xC5, 0x0C };

//Global Variables to check what data has been received.
int recvSfx;
int recvBg;
int recvBgVol = 50;
int recvSfxVol = 50;
int recvSfxPitch = 100;
bool dataReceived = false; 

//Runs when data has been received from ESP
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) 
{
  struct_message_all myResults; 
  //const uint8_t *mac_addr = info->src_addr;// Extract MAC address of sender
  memcpy(&myResults, incomingData, sizeof(myResults));

  int idx = myResults.id; //Need to see if Id = 6
  recvSfx = myResults.js; //Gives number of sfx to be played.
  recvBg = myResults.jc; //Gives number of background to be played.
  //recvBgVol = myResults. //Gives number for background volume to change to (1 to 101) 0 Means the volume isn't changing 
  //recvSfxVol = myResults. //Gives number for sfx volume to change to (1 to 101) 0 Means the volume isn't changing 
  //recvSfxPitch = myResults. //Gives number for sfx pitch to change to (95 to 110) 0 Means the pitch isn't changing 
  dataReceived = true;
}

//Initiate Sound effect voice structure.
void initVoices(SfxVoice &v)
{
  v.decoder = new WAVDecoder();
  v.memory = new MemoryStream(nullptr,0,false);
  v.stream = new EncodedAudioStream(v.memory,v.decoder);
  //v.resample = new ResampleStream(*v.stream);

  v.volume = new VolumeStream(*v.stream);
  v.volume->setAudioInfo(info);
  v.volume->begin();

  v.stream->begin();
  v.decoder->begin();

  v.mixerIndex = mixer.add(*v.volume, 0);
  v.active = false;
  v.startTime = 0;
  v.endTime = 1300;
}


// Initalisation
void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  delay(1000);

  Serial.printf("Total heap: %d", ESP.getHeapSize());
  Serial.println("");
  Serial.printf("Free heap: %d", ESP.getFreeHeap());
  Serial.println("");
  Serial.printf("Total PSRAM: %d", ESP.getPsramSize());
  Serial.println("");
  Serial.printf("Free PSRAM: %d", ESP.getFreePsram());
  Serial.println("");

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Register Callbacks using explicit casting to prevent v3.0 errors
  //esp_now_register_send_cb((esp_now_send_cb_t)OnDataSent);
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
  bgFile = SD.open("/m_intro.wav");
  currentPath = "/m_disco02.wav";
  if(!bgFile)
  {
    Serial.println("Background music missing.");
    return;   
  }
  //Starts Background music stream
  bgStream.begin();
  //Starts background music volume, so it can be changed throughout
  bgVolume.setAudioInfo(info);
  bgVolume.begin();
  bgVolume.setVolume(0.70); //Start at half volume (100% volume == 1)
  bgIndex = mixer.add(bgVolume, 100);
  Serial.println("Background Music Start!");
  
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
  //Loading SFX Sounds based on STRUCT calling from Flash Memory
  for(int i = 0; i < totalSfxVoices; i++)
  {
    initVoices(sfx_voices[i]);
  } 
  //Initiatate PSRAM sound effects
  for(int i=0; i < numSounds; i++)
  {
    sounds[i].data = nullptr;
    sounds[i].length = 0;
  }
}

void loop() {
  //Keeps audio running
  copier.copy();

  if(dataReceived)
  {
    Serial.printf("Sfx Data received: ");
    Serial.println(recvSfx);
    Serial.printf("Bg Data received: ");
    Serial.println(recvBg);
    Serial.printf("Bg Vol Data received: ");
    Serial.println(recvBgVol);
    Serial.printf("Sfx Vol Data received: ");
    Serial.println(recvSfxVol);
    Serial.printf("Sfx Vol Pitch Data received: ");
    Serial.println(recvSfxPitch);
    //0 Means Nothing, Don't use that, it's a null state meaning empty
    //If the background volume is changing
    if(recvBgVol >= 1 && recvBgVol <= 101)
    {
      Serial.println("Background Volume Change.");
      changeBgVol(recvBgVol-1);
    }
    //If The sound effect volume is changing
    if(recvSfxVol >= 1 && recvSfxVol <= 101)
    {
      Serial.println("Sound Effect Volume Change.");
      changeSfxVol(recvSfxVol-1);
    }
    //If the sound effect pitch is changing
    if(recvSfxPitch >= 95 && recvSfxPitch <= 110)
    {
      Serial.println("Sound Effect Pitch Change.");
      changeSfxPitch(recvSfxPitch);
    }
    // Play sound effects
    if(recvSfx >= 1 && recvSfx < 4)
    {
      Serial.println("Sound Effect Playing.");
      playSfx(recvSfx);
    }
    // 7 to 10 are music, 11 is silence
    if(recvBg >= 7 && recvBg < 12)
    {
      Serial.println("Background Change.");
      playBg(recvBg,0);
    }
    
    dataReceived = false;
  }

  //Detect End of Background Music
  if(!bgFile.available() || bgFile.available() < 2048)
  {
    //Serial.println("BG music ended");
    playBg(-1,1); //Not passing in new audio (-1), confirmed its looping instead (1)
  }
  //Deactivating any sound effects that have ended
  for(int i = 0; i < totalSfxVoices; i++)
  {
    SfxVoice &v = sfx_voices[i];
    if(v.active && millis() - v.startTime > v.endTime)
    {
      v.active = false;
      mixer.setWeight(v.mixerIndex,0);
    }
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

void loadGameSounds(GameStates current)
{
  currentGS = current;
  freeSounds();
  switch (current)
  {
    case MENU:
    //Selection Jinggles 
      loadSound("/sfxJump.wav",sounds[0]); //Jump Rope
      loadSound("/sfxSimon.wav",sounds[1]); //Simon Says
      loadSound("/sfxCalib.wav",sounds[2]); //Calibrations / Exercise
    break;
    case JUMP: 
    //Success, partial and fail
      loadSound("/sfxSuccess.wav",sounds[0]); //Success
      loadSound("/sfxPartial.wav",sounds[1]); //Partial
      loadSound("/sfxFail.wav",sounds[2]); //Fail
    break;
    case SIMON: 
    //Simon Says incrementing Sounds
      loadSound("/sfxS1.wav",sounds[0]);
      loadSound("/sfxS2.wav",sounds[1]);
      loadSound("/sfxS3.wav",sounds[2]);
      loadSound("/sfxS4.wav",sounds[3]);
      loadSound("/sfxS5.wav",sounds[4]);
      loadSound("/sfxS6.wav",sounds[5]);
      loadSound("/sfxS7.wav",sounds[6]);
    break;
    case CALIBRATION: 
    //Sound effects for all exercises
      loadSound("/sfxS1.wav",sounds[0]); //Start Exercise
      loadSound("/sfxS1.wav",sounds[1]); //End Exercise
      loadSound("/sfxS1.wav",sounds[2]); //Jump Indication
    break;
  }
  Serial.printf("Total PSRAM: %d", ESP.getPsramSize());
  Serial.println("");
  Serial.printf("Free PSRAM: %d", ESP.getFreePsram());
  Serial.println("");
  Serial.printf("Largest block: %u\n",
              heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
}


void playSfx(int sfx_playing)
{
  //Resets Variables for reasignment
  SfxVoice &v = sfx_voices[currentSfx];
  v.active = true;
  mixer.setWeight(v.mixerIndex,0);
  
  const uint8_t* currentData = nullptr;
  size_t currentLen = 0;
  Serial.println(sfx_playing);
  Serial.println("");
  //If playing from Flash Memory
  // if(sfx_playing <= 3)
  // {
  //   currentData = sfxList[sfx_playing].data;
  //   currentLen = sfxList[sfx_playing].len;
  //   //sfxDurationMs = sfxList[sfx_playing].ms;
  //   v.endTime = sfxList[sfx_playing].ms;
  // }
  //If playing from PSRAM Simon Says Tiles
  if(sfx_playing >= 0 && sfx_playing <= 6)
  {
    Serial.println(sfx_playing);
    Serial.println("Playing Simon Say sound effect");
    currentData = sounds[sfx_playing].data;
    currentLen = sounds[sfx_playing].length;
    if(currentGS == JUMP)
    {
      v.endTime = sfxList[sfx_playing+1].ms;
    }
    else
    {
      v.endTime = 1500;
    }
    
  }
  
  delete v.memory;
  v.memory = new MemoryStream((uint8_t*)currentData,currentLen,true,FLASH_RAM);

  v.stream->begin();
  v.decoder -> begin();

  v.startTime = millis();
  mixer.setWeight(v.mixerIndex,100);
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
  //Serial.println("Background Music Starting");
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

  //Load Audio if game has changed
  if (command.equalsIgnoreCase("menu")) {
    loadGameSounds(MENU);
    return;
  }
  else if (command.equalsIgnoreCase("jump")) {
    loadGameSounds(JUMP);
    return;
  }
  else if (command.equalsIgnoreCase("simon")) {
    loadGameSounds(SIMON);
    return;
  }
  else if (command.equalsIgnoreCase("calibration")) {
    loadGameSounds(CALIBRATION);
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
  if(menu == "home")
  {
    // --- Changing Serial Input Menu ---
    if (command.equalsIgnoreCase("bg")) 
    {
      menu = "bg";
      Serial.println("Now in BG Menu");
      return;
    }
    if (command.equalsIgnoreCase("sfx")) 
    {
      menu = "sfx";
      Serial.println("Now in SFX Menu");
      return;
    }
    //For testing purposes, a number is a sound effect, words is the background music
    if(isNumber)
    {
      fileIndex = command.toInt();
      Serial.println(fileIndex);
      int s_boundary = numSounds;
      //Checks if there's a possible sound effect in the number
      if(currentGS != SIMON){s_boundary = 3;}
      if(fileIndex > s_boundary) 
      {
        Serial.println("No SFX fit that number");
        return; 
      }
      currentSfx = -1;
      //Checks if there's a channel free in the mixer for the sound effect to play.
      for(int i = 0; i < totalSfxVoices; i++)
      {
        SfxVoice &v = sfx_voices[i];
        if(!v.active)
        {
          currentSfx = i;
          break;
        }
      }
      if(currentSfx == -1)
      {
        Serial.println("No space for new sfx");
        return;
      }
      playSfx(command.toInt()-1);
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
  }
  else if(menu == "bg")
  {
    if(isNumber)
    {
      changeBgVol(command.toInt());
      return;
    }
    if (command.equalsIgnoreCase("back")) 
    {
      menu = "home";
      Serial.println("In Main Menu");
      return;
    }
  }
  else if(menu == "sfx")
  {
    if(isNumber)
    {
      changeSfxVol(command.toInt());
      return;
    }
    if (command.equalsIgnoreCase("back")) 
    {
      menu = "home";
      Serial.println("In Main Menu");
      return;
    }
  }
  return;
}

void changeBgVol(int p_Vol)
{
  //Change Background Volume
  if(p_Vol < 0 || p_Vol > 100)
  {
    Serial.println("Invalid Volume Number, has to be between 0 and 100.");
    return;
  }
  Serial.println(p_Vol);
  float newVolume = float(p_Vol) / 100; //Divide number by 100 for decimal.
  Serial.println(newVolume);
  bgVolume.setVolume(newVolume);
}

void changeSfxVol(int p_Vol)
{
  //Change Background Volume
  if(p_Vol < 0 || p_Vol > 100)
  {
    Serial.println("Invalid Volume Number, has to be between 0 and 100.");
    return;
  }
  Serial.println(p_Vol);
  float newVolume = float(p_Vol) / 100; //Divide number by 100 for decimal.
  Serial.println(newVolume);
  sfxVolValue = newVolume;
  for (int i = 0; i < totalSfxVoices; i++)
  {
    sfx_voices[i].volume->setVolume(sfxVolValue);
  }
  //sfxVolume->setVolume(sfxVolValue);
}

void changeSfxPitch(int p_Pitch)
{
  float newPitch = float(p_Pitch) / 100;
  Serial.println(newPitch);
  sfxPitch = newPitch;
  // ResampleConfig cfg = sfxResample->defaultConfig();
  // cfg.step_size = sfxPitch;   // normal pitch
  // sfxResample->end();
  // sfxResample->begin(cfg);
}

//Load sounds from SD Card to PSRAM
bool loadSound(const char *filename, SoundEffect &sound)
{  
  //Open Sound Effect from files
  File file = SD.open(filename);
  if(!file)
  {
    Serial.println(filename);
    Serial.printf("Couldn't open %s\n", filename);
    return false;
  }

  //Allocate the size and data of file in a form that can be stored in PSRAM
  sound.length = file.size();
  sound.data = (uint8_t*)heap_caps_malloc(sound.length,MALLOC_CAP_SPIRAM);

  if(sound.data == nullptr)
  {
    Serial.printf("Couldn't allocat %u bytes\n", sound.length);
    file.close();
    return false;
  }

  file.read(sound.data, sound.length);
  file.close();

  Serial.printf("Loaded %s (%u bytes)\n", filename, sound.length);
  return true;
}

void freeSounds()
{
  for(int i = 0; i < numSounds; i++)
  {
    if(sounds[i].data)
    {
      free(sounds[i].data);
      sounds[i].data = nullptr;
      sounds[i].length = 0;
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


