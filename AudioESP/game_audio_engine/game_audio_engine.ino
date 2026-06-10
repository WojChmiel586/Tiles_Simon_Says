#include <driver/i2s.h>

#define SAMPLE_RATE 22050
#define MAX_CHANNELS 8

// -------------------------------
// Sound structure
// -------------------------------

struct Sound {
  const int16_t* data;
  uint32_t length;
  uint32_t position;
  bool playing;
  bool loop;
};

Sound channels[MAX_CHANNELS];

// -------------------------------
// Example audio data
// Replace with your own samples
// -------------------------------

const int16_t backgroundMusic[] = {0,200,400,200,0,-200,-400,-200};
const int musicLength = 8;

const int16_t jumpSound[] = {0,1000,2000,1000,0,-1000,-2000,-1000};
const int jumpLength = 8;

// -------------------------------
// I2S setup
// -------------------------------

void setupI2S() {

  i2s_config_t config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 256,
    .use_apll = false
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = 4,
    .ws_io_num = 5,
    .data_out_num = 6,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  i2s_driver_install(I2S_NUM_0, &config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
}

// -------------------------------
// Start music
// -------------------------------

void startMusic() {

  channels[0].data = backgroundMusic;
  channels[0].length = musicLength;
  channels[0].position = 0;
  channels[0].playing = true;
  channels[0].loop = true;
}

// -------------------------------
// Play sound effect
// -------------------------------

void playSound(const int16_t* data, int length) {

  for(int i=1;i<MAX_CHANNELS;i++) {

    if(!channels[i].playing) {

      channels[i].data = data;
      channels[i].length = length;
      channels[i].position = 0;
      channels[i].playing = true;
      channels[i].loop = false;

      break;
    }
  }
}

// -------------------------------
// Audio mixer
// -------------------------------

void audioTask(void* parameter) {

  size_t bytes_written;

  while(true) {

    int32_t mix = 0;
    int active = 0;

    for(int i=0;i<MAX_CHANNELS;i++) {

      if(channels[i].playing) {

        mix += channels[i].data[channels[i].position++];
        active++;

        if(channels[i].position >= channels[i].length) {

          if(channels[i].loop)
            channels[i].position = 0;
          else
            channels[i].playing = false;
        }
      }
    }

    if(active > 0)
      mix /= active;

    int16_t output = mix;

    i2s_write(I2S_NUM_0, &output, sizeof(output), &bytes_written, portMAX_DELAY);
  }
}

// -------------------------------
// Setup
// -------------------------------

void setup() {

  setupI2S();

  startMusic();

  xTaskCreatePinnedToCore(
    audioTask,
    "audio",
    4096,
    NULL,
    1,
    NULL,
    0
  );
}

// -------------------------------
// Game loop example
// -------------------------------

void loop() {

  static unsigned long lastTrigger = 0;

  if(millis() - lastTrigger > 2000) {

    playSound(jumpSound, jumpLength);

    lastTrigger = millis();
  }
}