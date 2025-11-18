#ifndef Tile_h
#define Tile_h
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

class Tile
{
  public:
  Tile();
  Tile(int pin);
  Adafruit_NeoPixel Strip();
  void SetColour();

  void begin();
  void setColour(uint32_t c);
  void light();
  void clear();
  bool isPressed();
  int getToeSensor();
  int getHeelSensor();
  Adafruit_NeoPixel strip;
  private:


  int _led_count;
  int _pin;
  int _toeSensor;
  int _heelSensor;
  int threshold = 400;
  uint32_t colour = 0;

};
#endif