#ifndef Tile_h
#define Tile_h
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <memory>
#include <vector>

#define NUM_PIXELS 61

class Tile
{
enum LEDsections : byte {
  TOP_HALF,
  BOTTOM_HALF,
  LEFT_HALF,
  RIGHT_HALF,
  CENTRE_LINE,
  TOP_LEFT,
  TOP_RIGHT,
  BOTTOM_RIGHT,
  BOTTOM_LEFT
};

struct LEDSegment 
{
  int startLED;
  int amountLED;
};

  public:
  //static
  static LEDSegment Q1[]; 
  static LEDSegment Q2[]; 
  static LEDSegment Q3[];
  static LEDSegment Q4[];


  Tile();
  Tile(int pin);
  Adafruit_NeoPixel& Strip();
  void SetColour();

  void begin();
  void setColour(uint32_t c);
  void light();
  void light(uint32_t c);
  void lightPartially(LEDsections section);
  void clear();
  bool isPressed();
  void setSensors(int toeVal, int heelVal);
  int getToeSensor();
  int getHeelSensor();


  //Different sections of Tiles
  private:



  std::unique_ptr<Adafruit_NeoPixel> strip;
  int _led_count;
  int _pin;
  int _toeSensor;
  int _heelSensor;
  int threshold = 800;
  uint32_t colour = 0;

};
#endif