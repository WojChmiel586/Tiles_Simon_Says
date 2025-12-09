#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <esp_now.h>  //now the ESP-NOW stuff....
#include <WiFi.h>

#include "Board.h"
#include "ESPNowStruct.h"

#define PIN_NEO_PIXEL 17  // The ESP32 pin GPIO17 connected to NeoPixel-example
#define NUM_PIXELS 61     // The number of LEDs (pixels) on NeoPixel

int ledPins[16] = {9,15,38,42,10,16,37,41,11,17,36,40,12,18,35,39};
int lastHitTile = -1;
Board board;

// Structure for data
struct_message_all myData;

volatile bool newDataAvailable = false;
struct_message_all boardsStructBack[7];
struct_message_all boardsStructFront[7];

//COLOURS REGION
#pragma region 
uint32_t lime = 0x69FF0A;      // RGB: 105, 255, 10
uint32_t cyan = 0x00FFFF;      // RGB: 0, 255, 255
uint32_t blue = 0x0000FF;      // RGB: 0, 0, 255
uint32_t purple = 0xB419FF;    // RGB: 180, 25, 255
uint32_t magenta = 0xFF00FF;   // RGB: 255, 0, 255
uint32_t orange = 0xFF5F1E;    // RGB: 255, 95, 30
uint32_t left = 0x00FFFF;      // Cyan
uint32_t right = 0xFF00FF;     // Magenta
uint32_t green = 0x00FF00;     // RGB: 0, 255, 0
uint32_t yellow = 0xFFFF00;    // RGB: 255, 255, 0
uint32_t red = 0xFF0000;       // RGB: 255, 0, 0
uint32_t white = 0xFFFFFF;     // RGB: 255, 255, 255
uint32_t black = 0x000000;     // RGB: 0, 0, 0
#pragma endregion

//------------------------------------ESP NOW STUFF--------------------------------------------------------------------------------------//

// REPLACE WITH YOUR RECEIVER MAC Address = button _audio ESP
uint8_t broadcastAddress1[] = { 0x24, 0xEC, 0x4A, 0x00, 0x92, 0xF8 };  //send to esp with buttons
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
  char macStr[18];
  // copy incoming into myData
  memcpy(&myData, incomingData, sizeof(myData));

  Serial.print("Received ID: ");
  Serial.println(myData.id);  // Check what IDs are actually coming in

  int idx = myData.id -1;

    if (idx < 0 || idx >= 7) 
    {
    Serial.print("ERROR: Invalid board ID received: ");
    Serial.println(myData.id);
    return;
    }
  boardsStructBack[idx] = myData;

  newDataAvailable = true;
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
   // 1. Initialize Serial FIRST (for debugging)
  Serial.begin(115200);
  delay(100);  // Give serial time to initialize
  Serial.println("\n\nStarting setup...");

  // 2. Initialize data structures
  memset(boardsStructBack, 0, sizeof(boardsStructBack));
  memset(boardsStructFront, 0, sizeof(boardsStructFront));
  
  // 3. Initialize WiFi BEFORE ESP-NOW
  Serial.println("Initializing WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();  // Ensure clean state
  delay(100);
  
  // 4. Initialize ESP-NOW
  Serial.println("Initializing ESP-NOW...");
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  Serial.println("ESP-NOW initialized successfully");

  // 5. Register callbacks
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));

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

  // 7. Initialize Board and NeoPixels LAST
  Serial.println("Initializing board and LEDs...");
  board.begin(ledPins);
  board.assignColours();
  board.clearAll();
  
  Serial.println("Setup complete!");
}

//=============================================================================================================
void loop() {
  if(newDataAvailable)
  {
    newDataAvailable = false;
    memcpy(boardsStructFront, boardsStructBack, sizeof(boardsStructBack));

    board.updateFromESPNOW(boardsStructFront);

    int pressedTile = board.pressedTile();

    if (pressedTile != lastHitTile)
    {
      if (lastHitTile >= 0)
      {
        board.clear(lastHitTile);
      }
      if (pressedTile >= 0)
      {
        board.light(pressedTile);
      }
      lastHitTile = pressedTile;
    }
  }

  //SIMON SAYS LOGIC:
  //1. RANDOMISE A NUMBER BETWEEN 0 and 15
  //2. ADD THE NUMBER TO A SEQUENCE ARRAY
  //3. LIGHT UP THE FIRST TILE IN A SEQUENCE
  //4. WAIT 300MS
  //5. TURN OFF THE PREVIOUS LIGHT (OPTIONAL DEPENDING ON DIFFICULTY)
  //6. TURN ON THE NEXT LIGHT IN THE SEQUENCE
  //7. REPEAT 4-6 UNTIL NO MORE NUMBERS IN THE SEQUENCE
  //8. 



}  //end of loop

//---------------------------------------FUNCTIONALITY-------------------------------------------------//

