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

// ------- Defining Varaibles and Functions for Audio Mixer ------

// ---- Audio Objects
I2SStream i2s;
InputMixer<int16_t> mixer;
StreamCopy copier(i2s,mixer,AUDIO_BUFFER_SIZE);
AudioInfo info(44100,2,16); //Rate of the WAV Files

// ---- Background Music ----
File bgFile;
WAVDecoder bgDecoder;
EncodedAudioStream bgStream(&bgFile,&bgDecoder);
VolumeStream bgVolume(bgStream);

int bgIndex = -1;
float bgVol = 0.30;
String currentPath;

// ---- Sound effects ----
SfxVoice sfx_voices[2]; //Amount of Sound effcts that could play at once.
int totalSfxVoices = 2;
int currentSfx = 0; //Acts as a pointer
bool isSfxPlaying = false;
float sfxVolValue = 0.70;

//Sound Effect start and end times (Milliseconds).
static uint32_t sfxStartTime = 0;
uint32_t sfxDurationMs = 2000;

//Sound Effects (PSRAM)
int numSounds = 7;
SoundEffect sounds[7]; //7 Max if each wav is 258kb (Total can't go over 2mb)
//Debugging Menu
String menu = "home";

// ---- Game States if things are to be loaded within the code ----
//enum GameStates {MENU,JUMP,SIMON,CALIBRATION};
//GameStates currentGS = MENU;

// ---- Audio Files structure 

struct AudioFile {
  String name;      // Base name without extension
  String wavPath;   // Full path to WAV file (if exists)
  bool hasWav;
};

//Total audio files within the SD card and one for just the music
AudioFile audioFiles[100];
int totalAudioFiles = 0;
AudioFile musicFiles[15];
int totalMusicFiles = 0;
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
int recvBgVol = 30;
int recvSfxVol = 70;
int recvBgLooping = 0;
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
  recvBgVol = myResults.sd; //Gives number for background volume to change to (1 to 101) 0 Means the volume isn't changing 
  recvSfxVol = myResults.dA; //Gives number for sfx volume to change to (1 to 101) 0 Means the volume isn't changing
  recvBgLooping = myResults.dB; //Gives true or false if needing to cut to silence afterwards. (1 = cut to silence)
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


// ---- Initalisation ----
void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  delay(1000);

  // ---- Initialize ESP-NOW
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

  // ---- Setup status LED
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

  // ---- Set up background music --
  //background music comes from the SD Card because they're larger files,
  // ---- Have to be WAV ---- or it wont work with the mixer.
  bgFile = SD.open("/m_intro.wav");
  currentPath = "/m_main_a.wav";
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
  bgVolume.setVolume(bgVol);
  bgIndex = mixer.add(bgVolume, 100); //Weight of mixer 0 = off, 100 = on.
  Serial.println("Background Music Start!");
  
}
void audioSetup() 
{
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
}

void loop() {
  //Keeps audio running
  copier.copy();

  // If the ESP has recieved a signal
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
    //0 Means Nothing, Don't use that, it's a null state meaning empty
    // ---- Background Volume Changing
    if(recvBgVol >= 1 && recvBgVol <= 101)
    {
      Serial.println("Background Volume Change.");
      changeBgVol(recvBgVol-1);
    }
    // ---- Sound Effect Volume Change
    if(recvSfxVol >= 1 && recvSfxVol <= 101)
    {
      Serial.println("Sound Effect Volume Change.");
      changeSfxVol(recvSfxVol-1);
    }
    // ----- Play sound effects
    if(recvSfx >= 1 && recvSfx < 16)
    {
      Serial.println("Sound Effect Playing.");
      playSfx(recvSfx);
    }
    // ---- Play Background Music
    if(recvBg >= 1 && recvBg < totalMusicFiles)
    {
      Serial.println("Background Change.");
      playBg(recvBg,0,recvBgLooping);
    }
    
    dataReceived = false;
  }

  //Detect End of Background Music
  if(!bgFile.available() || bgFile.available() < 2048)
  {
    //Serial.println("BG music ended");
    playBg(-1,1,0); //Not passing in new audio (-1), confirmed its looping instead (1)
  }

  //Deactivating any sound effects that have ended
  for(int i = 0; i < totalSfxVoices; i++)
  {
    SfxVoice &v = sfx_voices[i];
    //Has an end time which is slightly shorter than the file's actual duration.
    //This is done to avoid the click that appears when closing a file.
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

void playSfx(int sfx_playing)
{
  //Resets Variables for reasignment
  SfxVoice &v = sfx_voices[currentSfx];
  v.active = true;
  mixer.setWeight(v.mixerIndex,0);

  v.volume->setVolume(sfxVolValue);
  //Manual adjustment of volume for louder sound effects
  if(sfx_playing >= 4 && sfx_playing <= 6) // Selecting game nosises quite loud so this dampens it
  {
    v.volume->setVolume(sfxVolValue*0.46);
  }  
  // Empty variables to hold the new sound effect info
  const uint8_t* currentData = nullptr;
  size_t currentLen = 0;

  //If playing from Flash Memory
  if(sfx_playing <= sfxAmount)
  {
    currentData = sfxList[sfx_playing].data;
    currentLen = sfxList[sfx_playing].len;
    v.endTime = sfxList[sfx_playing].ms;
  }
  //If playing from PSRAM
  if(sfx_playing >= 11 && sfx_playing <= 17)
  {
    currentData = sounds[sfx_playing-10].data;
    currentLen = sounds[sfx_playing-10].length;
    v.endTime = 1500;
  }
  // ---- Deleting to avoid memory leaking / stacking up of the pointers
  //They have to be pointers as the version of Audio Tool that's being used
  //Doesn't have the functions to allow different methods of changing sound effects.
  delete v.memory;
  v.memory = new MemoryStream((uint8_t*)currentData,currentLen,true,FLASH_RAM);

  v.stream->begin();
  v.decoder -> begin();

  //Restarting timer
  v.startTime = millis();
  mixer.setWeight(v.mixerIndex,100);
  isSfxPlaying = true;
}

void playBg(int bg_playing, int looping, int cutting)
{
  String pathToPlay;
  // Looping music
  if(looping)
  {
    pathToPlay = currentPath;
  }
  // Changing Music
  else
  {
    if(bg_playing < 0 || bg_playing >= totalAudioFiles) {return;}
    bgVolume.setVolume(bgVol);
    //Adjusting volume if a quieter track is playing
    if(bg_playing == 3) //Whatever Value Simon Says Music is in the Music list
    {
      bgVolume.setVolume(bgVol * 1.8);
    }
    //If the file being played exists and has as WAV file
    if(musicFiles[bg_playing].hasWav)
    {
      pathToPlay = musicFiles[bg_playing].wavPath;
      currentPath = pathToPlay;
    }
    else
    {
      Serial.println("Error: No Valid Audio File Found.");
      return; 
    }
    //What the music will loop round to afterwards.
    if(bg_playing == 1)
    {
      currentPath = "/m_main_a.wav";
    }
    if(cutting == 1)
    {
      currentPath = "/m_silence.wav";
    }
  }
  //Changing the track in the mixer.
  mixer.setWeight(bgIndex,0); //Have to mute to avoid clicks or distortion whilst changing
  File newFile = SD.open(pathToPlay.c_str());
  if(!newFile){Serial.println("Missing Bg Music");}

  bgFile.close();
  bgFile = newFile;
  //Have to begin and end stream / decoder to avoid distortion
  bgDecoder.begin();
  bgStream.end();
  bgStream = EncodedAudioStream(&bgFile, &bgDecoder);
  bgStream.begin();

  mixer.setWeight(bgIndex,100);
  //Serial.println("Background Music Starting");
}

void processSerialCommand(String command) {
  command.trim();
  //If there's no command
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

  // ----- This is a menu system for the Serial Monitor -----
  // "home" is where you can call sound effects (Number) or the background track (Name).
  // "bg" is where you adjust the baackground's volume, type "back" to return to home.
  // "sfx" is where you adjust the sound effect's volume, type "back" to return to home.
  // "bgself" is where you can select the background track via numbers, type "back" to return to home.
  // This can all be removed / simplified once the final ESP is ready as this has been for testing purposes
  // And is only used with the Serial Monitor.
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
      if (command.equalsIgnoreCase("bgsel")) 
      {
        menu = "bgsel";
        Serial.println("Now in bg selection Menu");
        return;
      }
    //For testing purposes, a number is a sound effect, words is the background music
    if(isNumber)
    {
      fileIndex = command.toInt();
      //Checks if there's a possible sound effect in the number
      if(fileIndex > sfxAmount + 7) // + 7 for the slots in PSRAM. 
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
      playSfx(command.toInt());
    }
    else
    {
      //Finds the background music
      for (int i = 0; i < totalMusicFiles; i++)
      {
        if(musicFiles[i].name.equalsIgnoreCase(command))
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
      playBg(fileIndex,0,0);
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
  else if(menu == "bgsel")
  {
    if (command.equalsIgnoreCase("back")) 
    {
      menu = "home";
      Serial.println("In Main Menu");
      return;
    }
    fileIndex = command.toInt();
    //Checks if there's a possible sound effect in the number
    if(fileIndex > totalMusicFiles) 
    {
      Serial.println("No BG fit that number");
      return; 
    }
    playBg(fileIndex,0,0);
  }
  // ---- This is the end of the serial monitor menu system ----
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
  bgVol = float(p_Vol) / 100; //Divide number by 100 for decimal.
  bgVolume.setVolume(bgVol);
}

void changeSfxVol(int p_Vol)
{
  //Change Sound Effect Volume
  if(p_Vol < 0 || p_Vol > 100)
  {
    Serial.println("Invalid Volume Number, has to be between 0 and 100.");
    return;
  }
  sfxVolValue = float(p_Vol) / 100; //Divide number by 100 for decimal.
  //Change volume for all sound effects in list
  for (int i = 0; i < totalSfxVoices; i++)
  {
    sfx_voices[i].volume->setVolume(sfxVolValue);
  }
}

void scanAudioFiles() {
  File root = SD.open("/");
  if (!root) {
    Serial.println("Failed to open root directory");
    return;
  }
  
  totalAudioFiles = 0;
  //Makes sure the first spot is empty for signal receival. 
  musicFiles[0].name = "Empty";
  musicFiles[0].hasWav = false;
  musicFiles[0].wavPath = "";
  totalMusicFiles = 1;

  File file = root.openNextFile();
  
  while (file) {
    String fileName = String(file.name());
    //Checks if the current file is one for background music
    bool currentMus = false;
    if(fileName.charAt(0) == 'm')
    {
      Serial.println("Music Found");
      currentMus = true;
    }
    //Adds file to audio IF its a WAV file. 
    if (!file.isDirectory() && (fileName.endsWith(".wav") || fileName.endsWith(".WAV")))
    {
      // Get base name without extension
      String baseName = fileName;
      int lastDot = baseName.lastIndexOf('.');
      if (lastDot > 0) {
        baseName = baseName.substring(0, lastDot);
      }

      String fullPath = "/" + fileName;
     
      // Create new entry
      if (totalAudioFiles < 100) 
      {
        //Adds it to list of all WAV files in SD card
        audioFiles[totalAudioFiles].name = baseName;
        audioFiles[totalAudioFiles].hasWav = true;
        audioFiles[totalAudioFiles].wavPath = fullPath;
        totalAudioFiles++;
        //If it's background music, adds it to the music list.
        if(currentMus)
        {
          musicFiles[totalMusicFiles].name = baseName;
          musicFiles[totalMusicFiles].hasWav = true;
          musicFiles[totalMusicFiles].wavPath = fullPath;
          totalMusicFiles++;
        }
      }
    }
    
    file.close();
    file = root.openNextFile();
  }
  
  root.close();
}

void listAllFiles() {
  Serial.println("\n===== MUSIC FILES ON SD CARD =====");
  Serial.printf("Total files: %d\n\n", totalMusicFiles);
  //Lists Music Files on SD Card
  for (int i = 0; i < totalMusicFiles; i++) {
    Serial.printf("[%d] %s", i, musicFiles[i].name.c_str()); //Change musicFiles to audioFiles to display all files on the SD card
    Serial.print(" (WAV)");
    Serial.println();
  }
  Serial.println("\n===== SOUND EFFECTS ON FLASH MEMORY / PSRAM =====");
  Serial.printf("Total files: %d\n\n", sfxAmount);
  //Lists Sound Effects within Flash Memory
  for (int j = 0; j < sfxAmount; j++)
  {
    Serial.printf("[%d] %s",j,sfxList[j].name);
    Serial.println();
  }
  Serial.println("\n===================================");
  Serial.println("Type a number to play from RAM and a name to play from SD:");
  Serial.println("Type 'list' to show all files again\n");
}