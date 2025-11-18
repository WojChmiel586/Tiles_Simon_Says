#ifndef Tile_h
#define Tile_h
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

class Tile
{
  public:
  Tile(int pin);
  ~Tile();
  Adafruit_NeoPixel Strip();
  void SetColour();
  private:

  Adafruit_NeoPixel _strip;
  int _led_count;
  int _pin;
  int _toeSensor;
  int _heelSensor;

};


// the #include statement and code go here...

#endif