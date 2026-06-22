#pragma once

// uniform structure for all data messages
typedef struct struct_message_all {  // sender/receiver must match structure
  int id;                            // unique sender ID: yellobyte ESP = 1, game ESP = 2
  int t;                             // can be used for airtime
  int b;                             // can be used for buttonInput
  int jc;                            // can be used for jumpCount
  int js;                            // can be used for jumpState
  int sd;                            // can be used for stepDelay
  int dA;                            // left toe sensor
  int dB;                            // left heel sensor
  int eA;                            // right toe sensor
  int eB;                            // right heel sensor
  int fA;
  int fB;
  int gA;
  int gB;
} struct_message_all;    // Create an outgoing struct_message from yellobyte ESP called myGame