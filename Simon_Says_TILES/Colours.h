#ifndef COLOURS_H
#define COLOURS_H

#include <stdint.h>

struct Colours
{
  static constexpr uint32_t lime = 0x69FF0A;     // RGB: 105, 255, 10
  static constexpr uint32_t cyan = 0x00FFFF;     // RGB: 0, 255, 255
  static constexpr uint32_t blue = 0x0000FF;     // RGB: 0, 0, 255
  static constexpr uint32_t purple = 0xB419FF;   // RGB: 180, 25, 255
  static constexpr uint32_t magenta = 0xFF00FF;  // RGB: 255, 0, 255
  static constexpr uint32_t orange = 0xFF5F1E;   // RGB: 255, 95, 30
  static constexpr uint32_t left = 0x00FFFF;     // Cyan
  static constexpr uint32_t right = 0xFF00FF;    // Magenta
  static constexpr uint32_t green = 0x00FF00;    // RGB: 0, 255, 0
  static constexpr uint32_t yellow = 0xFFFF00;   // RGB: 255, 255, 0
  static constexpr uint32_t red = 0xFF0000;      // RGB: 255, 0, 0
  static constexpr uint32_t white = 0xFFFFFF;    // RGB: 255, 255, 255
  static constexpr uint32_t black = 0x000000;     // RGB: 0, 0, 0
};

#endif