#include "Tile.h"

std::vector<Tile::LEDSegment> Tile::Q1 = 
  {
    {20, 11},
    {0, 5},
    {51, 5}
  };
std::vector <Tile::LEDSegment> Tile::Q2 = 
{
  {5, 16},
  {51, 5}
};
std::vector <Tile::LEDSegment> Tile::Q3 =
{
  {31, 10},
  {0,5},
  {56,5}
};
std::vector <Tile::LEDSegment> Tile::Q4 =
{
  {41,10},
  {5,5},
  {56,5}
};

std::vector <Tile::LEDSegment> Tile::HalfL =
{
  {20,21},
  {51,10}
};

std::vector <Tile::LEDSegment> Tile::HalfR =
{
  {10,11},
  {51,10},
  {41,10}
};

std::vector <Tile::LEDSegment> Tile::HalfUp =
{
  {0,31}
};

std::vector <Tile::LEDSegment> Tile::HalfDown =
{
  {0,10},
  {30,21}
};

std::vector <Tile::LEDSegment> Tile::Outline =
{
  {10,41}
};

std::vector <Tile::LEDSegment> Tile::CentreHoriz =
{
  {0,10}
};

std::vector <Tile::LEDSegment> Tile::CentreVert =
{
  {51,10}
};

std::vector <Tile::LEDSegment> Tile::Cross =
{
  {51,10},
  {0,10}
};

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

void Tile::light(uint32_t c)
{
  for (int i = 0; i < strip->numPixels(); i++)
  {
    strip->setPixelColor(i, c);
  }
  strip->show();
}

void Tile::lightPartially(LEDsections section, uint32_t c)
{
  clear();
  std::vector<LEDSegment> segments;
  uint32_t color;
  if(c == 0)
  {
    color = colour;
  }
  else
  {
    color = c;
  }

  switch (section) 
  {
  case TOP_HALF:
  {
    segments = Tile::HalfUp;
  }
  break;
  case BOTTOM_HALF:
  {
    segments = Tile::HalfDown;
  }

  break;
  case LEFT_HALF:
  {
    segments = Tile::HalfL;
  }

  break;
  case RIGHT_HALF:
  {
    segments = Tile::HalfR;
  }

  break;
  case OUTLINE:
  {
    segments = Tile::Outline;
  }
  break;

  case CENTRE_LINE_VERTICAL:
  {
    segments = Tile::CentreVert;
  }

  break;
  case CENTRE_LINE_HORIZONTAL:
  {
    segments = Tile::CentreHoriz;
  }

  break;
  case CROSS:
  {
    segments = Tile::Cross;
  }

  break;
  case TOP_LEFT:
  {
    segments = Tile::Q1;
  }

  break;
  case TOP_RIGHT:
  {
    segments = Tile::Q2;
  }
  break;
  case BOTTOM_RIGHT:
  {
    segments = Tile::Q4;
  }

  break;
  case BOTTOM_LEFT:
  {
    segments = Tile::Q3;
  }

  break;
  default:

  break;
  }

    for(auto& segment : segments)
    {
      for(int i = 0; i < segment.amountLED; i++)
      {
        strip->setPixelColor(segment.startLED + i, color);
      }
    }
    strip->show();

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