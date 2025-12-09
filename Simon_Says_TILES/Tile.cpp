#include "Tile.h"

Tile::Tile()
{
  strip = std::make_unique<Adafruit_NeoPixel>(0, 0, NEO_GRB + NEO_KHZ800);
  _pin = 0;
  _toeSensor = 0;
  _heelSensor = 0;
}

Tile::Tile(int pin)
{
  strip = std::make_unique<Adafruit_NeoPixel>(NUM_PIXELS, pin, NEO_GRB + NEO_KHZ800);
  _pin = pin;
  _toeSensor = 0;
  _heelSensor = 0;
}

Adafruit_NeoPixel& Tile::Strip()
{
  return *strip;
}

void Tile::begin()
{
  strip->begin();
  strip->show();
}

void Tile::setColour(uint32_t c)
{
  colour = c;
}

void Tile::light()
{
 for (int i = 0; i < strip->numPixels(); i++)
 {
    strip->setPixelColor(i, colour);
 }
  strip->show();
}

void Tile::lightPartially(LEDsections section)
{
  clear();
  switch (section) 
  {
  case TOP_HALF:
  
  break;
  case BOTTOM_HALF:

  break;
  case LEFT_HALF:

  break;
  case RIGHT_HALF:

  break;
  case CENTRE_LINE:

  break;
  case TOP_LEFT:

  break;
  case TOP_RIGHT:

  break;
  case BOTTOM_RIGHT:

  break;
  case BOTTOM_LEFT:

  break;
  default:

  break;
  }

}
void Tile::clear() 
{
  strip->clear();
  strip->show();
}

void Tile::setSensors(int toeVal, int heelVal)
{
  _toeSensor = toeVal;
  _heelSensor = heelVal;
}

bool Tile::isPressed() 
{
    int a = _toeSensor;
    int b = _heelSensor;
    return (a > threshold || b > threshold);
}

int Tile::getToeSensor() 
{
    return _toeSensor;
}

int Tile::getHeelSensor() 
{
    return _heelSensor;
}