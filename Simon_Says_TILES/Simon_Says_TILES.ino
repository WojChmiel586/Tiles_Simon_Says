#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <esp_now.h>  //now the ESP-NOW stuff....
#include <WiFi.h>
#include <vector>
#include <memory>

#include "Board.h"
#include "Game.h"
#include "Menu.h"
#include "SimonSays.h"
#include "JumpRope.h"
#include "Calibration.h"
#include "Colours.h"
#include "ESPNowStruct.h"

#define PIN_NEO_PIXEL 17  // The ESP32 pin GPIO17 connected to NeoPixel-example
#define NUM_PIXELS 61     // The number of LEDs (pixels) on NeoPixel

int ledPins[16] = { 9, 15, 38, 42, 10, 16, 37, 41, 11, 17, 36, 40, 12, 18, 35, 39 };
Board board;
//------------------------------------ESP NOW STUFF--------------------------------------------------------------------------------------//

// REPLACE WITH YOUR RECEIVER MAC Address = button and audio ESP
uint8_t broadcastAddress1[] = { 0xEC, 0xDA, 0x3B, 0x95, 0xC4, 0xE0 };  //send to esp with buttons
uint8_t broadcastAddress2[] = { 0x3C, 0x84, 0x27, 0x31, 0xA0, 0x3C };  //send to yellobyte esp for sounds
uint8_t broadcastAddress3[] = { 0x24, 0xEC, 0x4A, 0x00, 0x92, 0xF8 };  //send to results esp

// ---- Game instances --------------------------------------------------------
Game* simonSays   = new SimonSays(board);
Game* jumpRope    = new JumpRope(board);
Game* calibration = new Calibration(board);
Game* mainMenu    = new Menu(board);
Game* currentGame = nullptr;

//   1 → Calibration (selects exercise via setExercise, then Init)
//   2 → JumpRope
//   3 → SimonSays
static const int BUTTON_JUMPROPE    = 2;
static const int BUTTON_SIMONSAYS   = 3;
static const int BUTTON_CALIBRATION = 1;  // switches to calibration mode; exercise set by 91,92,93,94,95,96,97,98

int  lastButtonValue = 0;   // previous frame's .b — for edge detection

// ---- Delta time ------------------------------------------------------------
unsigned long previousTime = 0;
unsigned long deltaTime    = 0;
// the setup routine runs once when you press reset:
void setup() {
  // 1. Initialize Serial FIRST (for debugging)
  Serial.begin(115200);
  delay(100);  // Give serial time to initialize
  Serial.println("\n\nStarting setup...");

    // 2. Initialize Board and NeoPixels
    board.begin(ledPins);
    board.clearAll();

    // 3. Initialize ESP-NOW through Board class
  if (!board.initESPNow()) {
    Serial.println("ESP-NOW initialization failed!");
    return;
  }

  // 4. Store peer addresses in Board
  board.setPeerAddresses(broadcastAddress1, broadcastAddress2, broadcastAddress3);

  // 5. Add peers through Board class
  if (!board.addPeer(broadcastAddress1)) {
    Serial.println("Failed to add peer 1");
    return;
  }
  Serial.println("Peer 1 (button ESP) added");

  if (!board.addPeer(broadcastAddress2)) {
    Serial.println("Failed to add peer 2");
    return;
  }
  Serial.println("Peer 2 (yellobyte ESP) added");

  if (!board.addPeer(broadcastAddress3)) {
    Serial.println("Failed to add peer 3");
    return;
  }
  Serial.println("Peer 3 (results ESP) added");

  Serial.println("Setup complete!");

  // Seed the random number generator
  randomSeed(analogRead(0));

  // Initialise game specific stuff
  simonSays->Init();
  jumpRope->Init();
  calibration->Init();
  mainMenu->Init();
  previousTime = millis();
  currentGame = mainMenu;   // default game on boot
}

//LOOP
void loop() 
{
  unsigned long currentTime = millis();
  deltaTime     = (currentTime - previousTime);
  previousTime  = currentTime;

  board.processRecievedData();

  // --- Read button input
  int buttonValue = board.getStructFront()[4].b;



  if (buttonValue != lastButtonValue)   // value changed this frame
  {
    Serial.print("Recieved a button press: ");
    Serial.println(buttonValue);
    lastButtonValue = buttonValue;

      if (buttonValue == 0)
      {
        currentGame = mainMenu;
        currentGame->Init();
      }
      // --- Game-select tier (0-5) ----------------------------------------
      else if (buttonValue == BUTTON_JUMPROPE)
      {
        Serial.println("Switching to JumpRope");
        currentGame = jumpRope;
        currentGame->Init();
      }
      else if (buttonValue == BUTTON_SIMONSAYS)
      {
        Serial.println("Switching to SimonSays");
        currentGame = simonSays;
        currentGame->Init();
      }
      else if (buttonValue == BUTTON_CALIBRATION)
      {
        Serial.println("Switching to Calibration");
        currentGame = calibration;
        static_cast<Calibration*>(calibration)->setExercise(91);
        currentGame->Init();
      }
      // --- Calibration exercise select (91-98) -----------------------------
      // Switches to Calibration and selects the exercise in one button press.
      // Values will be reassigned when button layout is finalised.
      else if (buttonValue >= 91 && buttonValue <= 98)
      {
        currentGame->HandleInput(buttonValue);
      }
  }

  // --- Run the active game --------------------------------------------------
  currentGame->Run(deltaTime);

}  //end of loop

//---------------------------------------TEST CODE-------------------------------------------------//


  // bool sectionTest = false;
  // unsigned long testInterval = 1000;
  // unsigned long testTimer = 0;
  // int section = 0;
  // if(currentMillis - testTimer >= testInterval)
  // {
  //   sectionTest = false;
  //   testTimer = currentMillis;
  //   section++;
  //   if(section >= 12)
  //   section = 0;
  // }
  // if (!sectionTest)
  // {
  //   Tile::LEDsections m_section = static_cast<Tile::LEDsections>(section);
  //   board.light(0, Colours::blue, m_section);
  //   board.light(1, Colours::blue, m_section);
  //   board.light(2, Colours::blue, m_section);
  //   board.light(3, Colours::blue, m_section);
  //   board.light(4, Colours::red, m_section);
  //   board.light(5, Colours::red, m_section);
  //   board.light(6, Colours::red, m_section);
  //   board.light(7, Colours::red, m_section);
  //   board.light(8, Colours::green, m_section);
  //   board.light(9, Colours::green, m_section);
  //   board.light(10, Colours::green, m_section);
  //   board.light(11, Colours::green, m_section);
  //   board.light(12, Colours::white, m_section);
  //   board.light(13, Colours::white, m_section);
  //   board.light(14, Colours::white, m_section);
  //   board.light(15, Colours::white, m_section);

  //   sectionTest = true;
  // }

