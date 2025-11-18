#include "Arduino.h"
#include "Tile.h"

Tile::Tile(){};
Tile::Tile(int pin)
{
  strip = Adafruit_NeoPixel(61, pin, NEO_GRB + NEO_KHZ800);
  _pin = pin;
}

Adafruit_NeoPixel Tile::Strip()
{
  return strip;
}

void Tile::begin()
{
  strip.begin();
  strip.show();
}

void Tile::setColour(uint32_t c)
{
  colour = c;
}

void Tile::light()
{
 for (int i = 0; i < strip.numPixels(); i++)
    strip.setPixelColor(i, colour);
    strip.show();
}

void Tile::clear() 
{
    for (int i = 0; i < strip.numPixels(); i++)
    strip.setPixelColor(i, 0);
    strip.show();
}

bool Tile::isPressed() 
{
    int a = analogRead(_toeSensor);
    int b = analogRead(_heelSensor);
    return (a > threshold || b > threshold);
}

int Tile::getToeSensor() 
{
    return analogRead(_toeSensor);
}

int Tile::getHeelSensor() 
{
    return analogRead(_heelSensor);
}