#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <esp_now.h>  //now the ESP-NOW stuff....
#include <WiFi.h>

#include "Board.h"
#include "Game.h"
#include "ESPNowStruct.h"

#define PIN_NEO_PIXEL 17  // The ESP32 pin GPIO17 connected to NeoPixel-example
#define NUM_PIXELS 61     // The number of LEDs (pixels) on NeoPixel

int ledPins[16] = { 9, 15, 38, 42, 10, 16, 37, 41, 11, 17, 36, 40, 12, 18, 35, 39 };
int lastHitTile = -1;
Board board;

// Structure for data
struct_message_all myData;

//Structure for results
struct_message_all myResults;

volatile bool newDataAvailable = false;
struct_message_all boardsStructBack[7];
struct_message_all boardsStructFront[7];

//COLOURS REGION
#pragma region
uint32_t lime = 0x69FF0A;     // RGB: 105, 255, 10
uint32_t cyan = 0x00FFFF;     // RGB: 0, 255, 255
uint32_t blue = 0x0000FF;     // RGB: 0, 0, 255
uint32_t purple = 0xB419FF;   // RGB: 180, 25, 255
uint32_t magenta = 0xFF00FF;  // RGB: 255, 0, 255
uint32_t orange = 0xFF5F1E;   // RGB: 255, 95, 30
uint32_t left = 0x00FFFF;     // Cyan
uint32_t right = 0xFF00FF;    // Magenta
uint32_t green = 0x00FF00;    // RGB: 0, 255, 0
uint32_t yellow = 0xFFFF00;   // RGB: 255, 255, 0
uint32_t red = 0xFF0000;      // RGB: 255, 0, 0
uint32_t white = 0xFFFFFF;    // RGB: 255, 255, 255
uint32_t black = 0x000000;    // RGB: 0, 0, 0
#pragma endregion

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
Game simonSays = Game(board);
std::vector<int> game_sequence;
std::vector<int> player_sequence;
int sequenceIdx = 0;
int prevSequenceLength = game_sequence.size();
int lastTile = -1;
unsigned long lastTileUpdate = 0;
const unsigned long sequenceInterval = 800;
unsigned long blinkTime = 0;
const unsigned long blinkInterval = 200;
unsigned long endWaitTime = 0;
const unsigned long endWaitInterval = 1000;
bool playerTurn = false;
bool playerFailed = false;
int finalScore = 0;



// the setup routine runs once when you press reset:--------------------------------------------------
void setup() {
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
  simonSays.Init();
  game_sequence.reserve(10);
  player_sequence.reserve(10);
  game_sequence.emplace_back(13);
}

//=============================================================================================================
void loop() {
  if (newDataAvailable) 
  {
    newDataAvailable = false;
    memcpy(boardsStructFront, boardsStructBack, sizeof(boardsStructBack));

    board.updateFromESPNOW(boardsStructFront);

    if(millis() - endWaitTime <= endWaitInterval && game_sequence.size() > 1)
    {
      return;
    }

    //SIMON SAYS LOGIC:
    //1. RANDOMISE A NUMBER BETWEEN 0 and 15 and add it to the array
    if (game_sequence.size() == prevSequenceLength) 
    {
      board.clearAll();
      //Add all possible tiles to a temporarly list <---- THIS CAN BE EXPANDED FOR HIGHER DIFFICULTIES
      std::vector<int> possibleTiles;
      int previous = game_sequence.back();
      if(previous + 1 <= 15 && previous + 1 >= 0 && previous % 4 != 3)
      {
        possibleTiles.emplace_back(previous + 1);
      }
      if(previous - 1 <= 15 && previous - 1 >= 0 && previous % 4 != 0)
      {
        possibleTiles.emplace_back(previous - 1);
      }
      if(game_sequence.back() + 4 <= 15 && game_sequence.back() + 4 >= 0)
      {
        possibleTiles.emplace_back(previous + 4);
      }
      if(game_sequence.back() - 4 <= 15 && game_sequence.back() - 4 >= 0)
      {
        possibleTiles.emplace_back(previous - 4);
      }

      //randomly choose one of these possible tiles and assign it to the sequence
      int nextTile = possibleTiles[random(0,possibleTiles.size())];
      game_sequence.emplace_back(nextTile);

      for (auto tile : game_sequence)
      {
        Serial.print("Tile in the sequence: " );
        Serial.println(tile);
      }
    }

    //3. LIGHT UP THE FIRST TILE IN A SEQUENCE AND WAIT FOR A SET TIME
    //4. TURN OFF THE PREVIOUS LIGHT (OPTIONAL DEPENDING ON DIFFICULTY)
    //5. TURN ON THE NEXT LIGHT IN THE SEQUENCE
    //6. REPEAT 3-5 UNTIL NO MORE NUMBERS IN THE SEQUENCE
    if (millis() - lastTileUpdate >= sequenceInterval && !playerTurn)
    {
      //GO THROUGH EACH TILE NUMBER IN SEQUENCE
      if (sequenceIdx < game_sequence.size()) {
        board.light(game_sequence.at(sequenceIdx));

        if (sequenceIdx > 0)
        {
          board.clear(game_sequence.at(sequenceIdx-1));
        }
        lastTileUpdate = millis();
        sequenceIdx++;
      }
      //WE FINISHED THE SEQUENCE
      else
      {
        //CLEAR THE BOARD LIGHTS
        board.clearAll();
        sequenceIdx = 0;
        playerTurn = true;
      }
    }

    //PLAYERS TURN LOGIC:
    //8. WAIT FOR PLAYER TO STEP ON TILE
    //9. IF CORRECT TILE STEPPED ON, WAIT FOR NEXT INPUT OTHERWISE END THE GAME
    //10 ONCE THE SEQUENCE IS COMPLETE, INDICATE A TILE FOR PLAYER TO STAND ON TO CONTINUE GAME
    //11 REPEAT UNTIL FAILURE
    if (playerTurn && !playerFailed) {
      int pressedTile = board.pressedTile();


      //CHECK IF ANY VALID TILE IS PRESSED
      if (pressedTile >= 0 && pressedTile < 16) {
        //CHECK IF TILE PRESSED CORRESPONDS TO THE TILE IN GAME SEQUENCE

        //CODE WHEN TILE MATCHES SEQUENCE
        if (pressedTile == game_sequence.at(sequenceIdx) && pressedTile != lastTile) {
          //Light up stepped on tile
          board.light(pressedTile);
          //Turn off the previous tile (if any exist)
          if(lastTile >= 0 && lastTile <= 15)
          {
            board.clear(lastTile);
          }
          sequenceIdx++;
          lastTile = pressedTile;

          if(sequenceIdx >= game_sequence.size())
          {
            //PLAYER FINISHED THE SEQUENCE SUCCESFULLY
            prevSequenceLength = game_sequence.size();
            //board.clearAll();
            playerTurn = false;
            sequenceIdx = 0;
            lastTileUpdate = 0;
            lastTile = -1;
            endWaitTime = millis();
          }
        }
        else if (pressedTile != game_sequence.at(sequenceIdx) && pressedTile != lastTile && sequenceIdx != 0) {
          //CODE WHEN TILE DOESN'T MATCH SEQUENCE
          //SOME SORT OF INDICATION OF FAILURE

          finalScore = game_sequence.size();
          //CLEAR BOARD AND RESTART
          board.clearAll();
          game_sequence.clear();
          game_sequence.emplace_back(13);
          sequenceIdx = 0;
          prevSequenceLength = 0;
          playerFailed = true;

          Serial.println("Player Lost");
          myResults.id = 6;
          myResults.eA = finalScore;
          esp_err_t result1 = esp_now_send(broadcastAddress3, (uint8_t *)&myResults, sizeof(myResults));
        }
      }
    }

    //END STATE FOR THE GAME WHEN PLAYER FAILS
    if(playerFailed)
    {
      //BLINK THE BOARD LIGHTS RED TO INDICATE FAILURE
      if (millis() - blinkTime >= blinkInterval)
      {
        board.blinkBoard(red);
        blinkTime = millis();
      }
      //BUTTON CONTROL OR SOMETHING TO RESTART THE GAME? FOR NOW WE NEED TO CUT POWER TO RESTART
    }


  }





}  //end of loop

//---------------------------------------FUNCTIONALITY-------------------------------------------------//
