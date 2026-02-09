#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <esp_now.h>  //now the ESP-NOW stuff....
#include <WiFi.h>

#include "Board.h"
#include "Game.h"
#include "SimonSays.h"
#include "Colours.h"
#include "ESPNowStruct.h"

#define PIN_NEO_PIXEL 17  // The ESP32 pin GPIO17 connected to NeoPixel-example
#define NUM_PIXELS 61     // The number of LEDs (pixels) on NeoPixel

int ledPins[16] = { 9, 15, 38, 42, 10, 16, 37, 41, 11, 17, 36, 40, 12, 18, 35, 39 };
Board board;
unsigned long currentMillis = 0;

// Structure for data
struct_message_all myData;

//Structure for results
struct_message_all myResults;

volatile bool newDataAvailable = false;
struct_message_all boardsStructBack[7];
struct_message_all boardsStructFront[7];

//------------------------------------ESP NOW STUFF--------------------------------------------------------------------------------------//

// REPLACE WITH YOUR RECEIVER MAC Address = button and audio ESP
uint8_t broadcastAddress1[] = { 0xEC, 0xDA, 0x3B, 0x95, 0xC4, 0xE0 };  //send to esp with buttons
uint8_t broadcastAddress2[] = { 0x3C, 0x84, 0x27, 0x31, 0xA0, 0x3C };  //send to yellobyte esp for sounds
uint8_t broadcastAddress3[] = { 0x24, 0xEC, 0x4A, 0x00, 0x92, 0xF8 };  //send to results esp

//int jumpCount; //number of jump to be sent defined later
int jumpState;  //variables - readings to be sent = 0,1,2,3 other/good/partial/fail jump

String success;  // if readings succcessfully delivered to esp buttons/audio


esp_now_peer_info_t peerInfo;  // store info about peer

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  //Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  if (status == 0) {
    success = "Delivery Success";
  } else {
    success = "Delivery Fail";
  }
  char macStr[18];
}

#pragma region  //NETWORKING
// callback function that will be executed when data is received
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len) {
  char macStr[18];
  // copy incoming into myData
  memcpy(&myData, incomingData, sizeof(myData));

  //Serial.print("Received ID: ");
  //Serial.println(myData.id);  // Check what IDs are actually coming in

  int idx = myData.id - 1;

  if (idx < 0 || idx >= 7) {
    Serial.print("ERROR: Invalid board ID received: ");
    Serial.println(myData.id);
    return;
  }
  boardsStructBack[idx] = myData;

  newDataAvailable = true;
}
#pragma endregion
enum : byte {
  Setup,
  Gameplay,
  Summary
} state = Setup;

//Game variables
Game* simonSays = new SimonSays(board);


// the setup routine runs once when you press reset:--------------------------------------------------
void setup() {
  // 1. Initialize Serial FIRST (for debugging)
  Serial.begin(115200);
  delay(100);  // Give serial time to initialize
  Serial.println("\n\nStarting setup...");





  // 6. Register peers
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Add peer 1 - button ESP
  memcpy(peerInfo.peer_addr, broadcastAddress1, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer 1");
    return;
  }
  Serial.println("Peer 1 added");

  // Add peer 2 - yellobyte ESP
  memcpy(peerInfo.peer_addr, broadcastAddress2, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer 2");
    return;
  }
  Serial.println("Peer 2 added");

  // Add peer 3 - results ESP
  memcpy(peerInfo.peer_addr, broadcastAddress3, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer 3");
    return;
  }
  Serial.println("Peer 3 added");

  // 7. Initialize Board and NeoPixels LAST
  Serial.println("Initializing board and LEDs...");
  board.begin(ledPins);
  board.assignColours();
  board.clearAll();

  Serial.println("Setup complete!");

  //8. Seed the random number generator
  randomSeed(analogRead(0));

  //9. Initialise game specific stuff
  simonSays->Init();
}

//=============================================================================================================
void loop() 
{
  currentMillis = millis();
  if (newDataAvailable) 
  {
    newDataAvailable = false;
    memcpy(boardsStructFront, boardsStructBack, sizeof(boardsStructBack));

    board.updateFromESPNOW(boardsStructFront);
  }

  simonSays->Run(millis() - currentMillis);
}  //end of loop

//---------------------------------------FUNCTIONALITY-------------------------------------------------//
