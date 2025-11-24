#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <esp_now.h>  //now the ESP-NOW stuff....
#include <WiFi.h>

#include "Board.h"
#include "ESPNowStruct.h"

#define PIN_NEO_PIXEL 17  // The ESP32 pin GPIO17 connected to NeoPixel-example
#define NUM_PIXELS 61     // The number of LEDs (pixels) on NeoPixel

int ledPins[16] = {9,15,38,42,10,16,37,41,11,17,36,40,12,18,35,39};

Board board;

// Structure for data
struct_message_all myData;

struct_message_all ESP1;
struct_message_all ESP2;
struct_message_all ESP3;
struct_message_all ESP4;
struct_message_all ESP6;
struct_message_all ESP7;

volatile bool newDataAvailable = false;
struct_message_all boardsStructBack[6];
struct_message_all boardsStructFront[6];

//COLOURS REGION
#pragma region 
Adafruit_NeoPixel strip10;
uint32_t lime = strip10.Color(105, 255, 10);    //not used for level
uint32_t cyan = strip10.Color(0, 255, 255);     //used for level 1
uint32_t blue = strip10.Color(0, 0, 255);       //used for  level 2
uint32_t purple = strip10.Color(180, 25, 255);  //used for level 3
uint32_t magenta = strip10.Color(255, 0, 255);  // used for level 4
uint32_t orange = strip10.Color(255, 95, 30);   // used for level 5
uint32_t left = strip10.Color(0, 255, 255);     //colour used for LEFT FOOT cyan
uint32_t right = strip10.Color(255, 0, 255);    // colour used for RIGHT FOOT magenta

uint32_t green = strip10.Color(0, 255, 0);      //used for goodJump
uint32_t yellow = strip10.Color(255, 255, 0);   //used for partialJump
uint32_t red = strip10.Color(255, 0, 0);        // used for badJump
uint32_t white = strip10.Color(255, 255, 255);  // used for endJump
uint32_t black = strip10.Color(0, 0, 0);        // used to switch off??
#pragma endregion

//------------------------------------ESP NOW STUFF--------------------------------------------------------------------------------------//

// REPLACE WITH YOUR RECEIVER MAC Address = button _audio ESP
uint8_t broadcastAddress1[] = { 0xEC, 0xDA, 0x3B, 0x95, 0xC4, 0xE0 };  //send to esp with buttons
uint8_t broadcastAddress2[] = { 0x3C, 0x84, 0x27, 0x31, 0xA0, 0x3C };  //send to yellobyte esp for sounds

//int jumpCount; //number of jump to be sent defined later
int jumpState;  //variables - readings to be sent = 0,1,2,3 other/good/partial/fail jump

String success;  // if readings succcessfully delivered to esp buttons/audio




esp_now_peer_info_t peerInfo;  // store info about peer

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) 
{
  Serial.print("\r\nLast Packet Send Status:\t");
  //Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  if (status == 0) {
    success = "Delivery Success";
  } else {
    success = "Delivery Fail";
  }
  char macStr[18];
}

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) 
{
  // copy incoming into myData
  memcpy(&myData, incomingData, sizeof(myData));
  int idx = myData.id -1;

  boardsStructBack[idx] = myData;

  newDataAvailable = true;

  // boardsStruct[idx].t = myData.t;
  // boardsStruct[idx].b = myData.b;
  // boardsStruct[idx].jc = myData.jc;
  // boardsStruct[idx].js = myData.js;
  // boardsStruct[idx].sd = myData.sd;
  // boardsStruct[idx].dA = myData.dA;
  // boardsStruct[idx].dB = myData.dB;
  // boardsStruct[idx].eA = myData.eA;
  // boardsStruct[idx].eB = myData.eB;
  // boardsStruct[idx].fA = myData.fA;
  // boardsStruct[idx].fB = myData.fB;
  // boardsStruct[idx].gA = myData.gA;
  // boardsStruct[idx].gB = myData.gB;
}

enum : byte {
  Setup,
  Gameplay,
  Summary
} state = Setup;

//Game variables



// the setup routine runs once when you press reset:--------------------------------------------------
void setup() 
{
  board.begin(ledPins);
  board.assignColours();
  board.clearAll();

  Serial.begin(115200);            // Initialize Serial Monitor
  WiFi.mode(WIFI_STA);             // Set device as a Wi-Fi Station

  if (esp_now_init() != ESP_OK) 
  {  // Init ESP-NOW
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Transmitted packet
  esp_now_register_send_cb(OnDataSent);

  // Register peers
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  // Add peer 1 - button ESP
  memcpy(peerInfo.peer_addr, broadcastAddress1, 6);

  if (esp_now_add_peer(&peerInfo) != ESP_OK) 
  {
    Serial.println("Failed to add peer");
    return;
  }

  // Add peer 2 - yellobyte ESP
  memcpy(peerInfo.peer_addr, broadcastAddress2, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK) 
  {
    Serial.println("Failed to add peer");
    return;
  }

  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));

}

//=============================================================================================================
void loop() {
  if(newDataAvailable)
  {
    noInterrupts();
    memcpy(boardsStructFront, boardsStructBack, sizeof(boardsStructBack));
    newDataAvailable = false;
  }
  board.updateFromESPNOW(boardsStructFront);
  board.light(board.pressedTile());

  Serial.print(board.tiles[9].getHeelSensor());

  //Gamestate based on buttonInput
  // 99 = Calibration
  // 0 = Reset
  // 1 = Entry Game level at 80%
  // 2 = Game at 80% + 5%
  // 3 = Game at 80% + 10%
  // 4 = Game at 80% + 15%
  // 5 = Game at 80% + 20%

}  //end of loop

//---------------------------------------FUNCTIONALITY-------------------------------------------------//

