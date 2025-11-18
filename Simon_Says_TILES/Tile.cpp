#include "Arduino.h"
#include "Tile.h"

Tile::Tile(int pin)
{
  _strip = Adafruit_NeoPixel(61, pin, NEO_GRB + NEO_KHZ800);
  _pin = pin;
}

Adafruit_NeoPixel Tile::Strip()
{
  return _strip;
}