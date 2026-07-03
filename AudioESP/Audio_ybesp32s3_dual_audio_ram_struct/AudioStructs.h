#pragma once
#include "AudioTools.h"

// ---- Sound Effects  ----

//Structure for how the sound effect runs in the mixer.
typedef struct SfxVoice 
{
  MemoryStream* memory;
  WAVDecoder* decoder;
  EncodedAudioStream* stream;
  VolumeStream* volume; 
  int mixerIndex;
  bool active;
  uint32_t startTime;
  uint32_t endTime;
} SfxVoice;

//For creating sound effects stored in PSRAM 
typedef struct SoundEffect
{
  uint8_t *data;
  size_t length;
} SoundEffect;