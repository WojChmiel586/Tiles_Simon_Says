#ifndef Tile_h
#define Tile_h
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <memory>
#include <vector>

#define NUM_PIXELS 61

class Tile
{
public:
enum LEDsections : byte {
  TOP_HALF,
  BOTTOM_HALF,
  LEFT_HALF,
  RIGHT_HALF,
  CENTRE_LINE_VERTICAL,
  CENTRE_LINE_HORIZONTAL,
  TOP_LEFT,
  TOP_RIGHT,
  BOTTOM_RIGHT,
  BOTTOM_LEFT,
  OUTLINE,
  CROSS,
  WHOLE
};

struct LEDSegment 
{
  int startLED;
  int amountLED;
};


  //static
  static std::vector <LEDSegment> Q1;
  static std::vector <LEDSegment> Q2; 
  static std::vector <LEDSegment> Q3;
  static std::vector <LEDSegment> Q4;
  static std::vector <LEDSegment> HalfL;
  static std::vector <LEDSegment> HalfR;
  static std::vector <LEDSegment> HalfUp;
  static std::vector <LEDSegment> HalfDown;
  static std::vector <LEDSegment> Outline;
  static std::vector <LEDSegment> CentreVert;
  static std::vector <LEDSegment> CentreHoriz;
  static std::vector <LEDSegment> Cross;


  Tile();
  Tile(int pin);
  Adafruit_NeoPixel& Strip();
  void SetColour();

  void begin();
  void setColour(uint32_t c);
  void light();
  void light(uint32_t c);
  void lightPartially(LEDsections section, uint32_t c = 0);
  void clear();
  bool isPressed();
  void setSensors(int toeVal, int heelVal);
  int getToeSensor();
  int getHeelSensor();

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