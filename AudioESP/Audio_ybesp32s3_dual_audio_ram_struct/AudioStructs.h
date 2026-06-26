#pragma once
#include "AudioTools.h"

// ---- Sound Effects  ----

typedef struct SfxVoice 
{
  MemoryStream* memory;
  WAVDecoder* decoder;
  EncodedAudioStream* stream;
  VolumeStream* volume;
  //ResampleStream* resample;
  int mixerIndex;
  bool active;
  uint32_t startTime;
  uint32_t endTime;
} SfxVoice;

//For PSRAM 
typedef struct SoundEffect
{
  uint8_t *data;
  size_t length;
} SoundEffect;