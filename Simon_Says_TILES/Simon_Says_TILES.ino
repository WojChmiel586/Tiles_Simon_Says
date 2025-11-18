#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "Tile.h"
#define PIN_NEO_PIXEL 17  // The ESP32 pin GPIO17 connected to NeoPixel-example
#define NUM_PIXELS 61     // The number of LEDs (pixels) on NeoPixel

Adafruit_NeoPixel strip1 = Adafruit_NeoPixel(61, 9, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip2 = Adafruit_NeoPixel(61, 15, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip3 = Adafruit_NeoPixel(61, 38, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip4 = Adafruit_NeoPixel(61, 42, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip5 = Adafruit_NeoPixel(61, 10, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip6 = Adafruit_NeoPixel(61, 16, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip7 = Adafruit_NeoPixel(61, 37, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip8 = Adafruit_NeoPixel(61, 41, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip9 = Adafruit_NeoPixel(61, 11, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip10 = Adafruit_NeoPixel(61, 17, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip11 = Adafruit_NeoPixel(61, 36, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip12 = Adafruit_NeoPixel(61, 40, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip13 = Adafruit_NeoPixel(61, 12, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip14 = Adafruit_NeoPixel(61, 18, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip15 = Adafruit_NeoPixel(61, 35, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip16 = Adafruit_NeoPixel(61, 39, NEO_GRB + NEO_KHZ800);

Tile tile1 = Tile(9);
Tile tile2 = Tile(15);
Tile tile3 = Tile(38);
Tile tile4 = Tile(42);
Tile tile5 = Tile(10);
Tile tile6 = Tile(16);
Tile tile7 = Tile(37);
Tile tile8 = Tile(41);
Tile tile9 = Tile(11);
Tile tile10 = Tile(17);
Tile tile11 = Tile(36);
Tile tile12 = Tile(40);
Tile tile13 = Tile(12);
Tile tile14 = Tile(18);
Tile tile15 = Tile(35);
Tile tile16 = Tile(39);

Tile tiles[] = {
tile1,
tile2,
tile3,
tile4,
tile5,
tile6,
tile7,
tile8,
tile9,
tile10,
tile11,
tile12,
tile13,
tile14,
tile15,
tile16
};
Adafruit_NeoPixel stripArray[] = { strip1, strip2, strip3, strip4, strip5, strip6, strip7, strip8, strip9, strip10, strip11, strip12, strip13, strip14, strip15, strip16 };

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

#include <esp_now.h>  //now the ESP-NOW stuff....
#include <WiFi.h>
// REPLACE WITH YOUR RECEIVER MAC Address = button _audio ESP
uint8_t broadcastAddress1[] = { 0xEC, 0xDA, 0x3B, 0x95, 0xC4, 0xE0 };  //send to esp with buttons
uint8_t broadcastAddress2[] = { 0x3C, 0x84, 0x27, 0x31, 0xA0, 0x3C };  //send to yellobyte esp for sounds

//int jumpCount; //number of jump to be sent defined later
int jumpState;  //variables - readings to be sent = 0,1,2,3 other/good/partial/fail jump

int airtime;      //incoming from upper sensor ESP
int buttonInput;  //from upper sensor ESP = gamelevel 0,99,1,2,3,4,5
int T1A;          //Tile 1 toe sensor strip
int T1B;          //Tile 2 heel sensor strip
int T2A;
int T2B;
int T3A;
int T3B;
int T4A;
int T4B;
int T5A;
int T5B;
int T6A;
int T6B;
int T7A;
int T7B;
int T8A;
int T8B;
int T9A;
int T9B;
int T10A;
int T10B;
int T11A;
int T11B;
int T12A;
int T12B;
int T13A;
int T13B;
int T14A;
int T14B;
int T15A;
int T15B;
int T16A;
int T16B;
int leftSensor;   //T10 summary data -still needed for game
int rightSensor;  //T11 summary data -still needed for game

String success;  // if readings succcessfully delivered to esp buttons/audio

// Structure for data
typedef struct struct_message_short {  // sender/receiver must match structure
  char a[32];
  int t;
  int b;
  int d;
  int e;
} struct_message_short;

struct_message_short myJump;   // Create a struct_message called myJump to be sent to button ESP
struct_message_short mySound;  // Create a struct_message called mySound to be sent to yellobyte
struct_message_short myData;   // Create an incoming struct_message called myData

esp_now_peer_info_t peerInfo;  // store info about peer

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  if (status == 0) {
    success = "Delivery Success";
  } else {
    success = "Delivery Fail";
  }
  char macStr[18];
}

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  memcpy(&myData, incomingData, sizeof(myData));
  airtime = myData.t;
  buttonInput = myData.b;
  leftSensor = myData.d;
  rightSensor = myData.e;
}

enum : byte {
  Setup,
  Gameplay,
  Summary
} state = Setup;

//Calibration variables
int exCounter = 0;    // we are counting up exercise parts completed, start at 0
int stepDelay = 500;  // smallest timing unit. 666 would be equivalent to taking 3 steps in 2 seconds at 90bpm
int balanceDelay;
int sideliftDelay;     //time for one side leg lift
int lungeDelay;        //time to do to one lunge
int squat1Delay;       //time to do one squat
int squat2Delay;       //time do to one squat with 2 pulses
int jumpDelay;         // time to do 3 jumps
int balanceScore = 0;  // time spent in balance for L, R, toes, heels
int mobScore = 0;      // mobility score from CLOCKFACE exercise, tapping tiles in time: max value is 7 x 4 = 28
int weightOn = 120;     // threshold for weight on a tile
int strengthScore = 0; // counting  10 lunges and 6 squats, max score is 16

//Game variables
int liftOff = 0;
int landing = 0;
int lineDelay = 550;  // default max airtime (100%) needs to be adjusted to be player-specific
int lineDelay1;
int lineDelay2;
int lineDelay3;
int lineDelay4;
int lineDelay5;
int resultDelay = 300;  //feedback on valid jump
int interJump = 3500;
int jumpCount = -1;  // counting set up to 6 then long break and restarting

unsigned long setupTime = 61000;                // was 10000 but too long do we need this?
unsigned long previousMillis = 0;               // do we need this?
const unsigned long interval = setupTime / 61;  // do we need this?

int pixelIndex = 0;

// the setup routine runs once when you press reset:--------------------------------------------------
void setup() {
  for (int i = 0; i < 16; i++) {
    stripArray[i].begin();
    stripArray[i].show();  //Initialize all pixels to 'off'
  }

  Serial.begin(115200);            // Initialize Serial Monitor
  WiFi.mode(WIFI_STA);             // Set device as a Wi-Fi Station
  if (esp_now_init() != ESP_OK) {  // Init ESP-NOW
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
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  // Add peer 2 - yellobyte ESP
  memcpy(peerInfo.peer_addr, broadcastAddress2, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
  //  esp_now_register_recv_cb(OnDataRecv);
}

//=============================================================================================================
void loop() {
  //Gamestate based on buttonInput
  // 99 = Calibration
  // 0 = Reset
  // 1 = Entry Game level at 80%
  // 2 = Game at 80% + 5%
  // 3 = Game at 80% + 10%
  // 4 = Game at 80% + 15%
  // 5 = Game at 80% + 20%
  airtime = myData.t;                                           // check the airtime received
  lineDelay1 = (airtime * 0.8 / 3.0);                           // 80% airtime / 3 lines = ~147ms (if airtime is 550)
  lineDelay2 = ((airtime * 0.8 / 3.0) + (airtime / 10 * 0.5));  // 80% + 5%  ~175ms
  lineDelay3 = ((airtime * 0.8 / 3.0) + (airtime / 10 * 1.0));  // 80% + 10% ~202ms
  lineDelay4 = ((airtime * 0.8 / 3.0) + (airtime / 10 * 1.5));  // 80% + 15% ~230ms
  lineDelay5 = ((airtime * 0.8 / 3.0) + (airtime / 10 * 2.0));  // 80% + 20% ~257ms
  Serial.print("airtime: ");
  Serial.print(airtime);
  buttonInput = myData.b;  //check gamelevel from button input received
  Serial.print(", game level: ");
  Serial.print(buttonInput);

  //should check the stepDelay received here (default is 500)
  balanceDelay = (stepDelay * 20);  //10s
  sideliftDelay = (stepDelay * 4);  //2s
  lungeDelay = (stepDelay * 4);     //2s
  squat1Delay = (stepDelay * 4);    //2s
  squat2Delay = (stepDelay * 6);    //3s
  jumpDelay = (stepDelay * 30);     //15s

  switch (buttonInput) {
    case 99:  //start calibration
      {
        if (exCounter < 32) {  // 0-31, 1 MIN WARMUP check if 32 L+R steps completed
          calib1();
        } else if (exCounter == 32) {  // if 32 warm up COMPLETE, GO TO 1 BALANCE, ->33
          cleanUp();
          calib2();
        } else if (exCounter > 32 && exCounter < 38) {  // if balance complete, go to 5 side lifts 33 ->37
          cleanUp();
          calib3();
        } else if (exCounter >= 38 && exCounter < 40) {  // if side lifts complete, go to 4x CLOCKFACE (2 counts) 38 ->39
          cleanUp();
          calib4();
        } else if (exCounter >= 40 && exCounter < 45) {  //if CLOCKFACE complete, go to 5x L/R lunges 40 -> 44
          cleanUp();
          calib5();
        } else if (exCounter >= 45 && exCounter < 48) {  //if lunges complete, go to 3x squats 45 -> 47
          cleanUp();
          calib6a();
        } else if (exCounter >= 48 && exCounter < 51) {  //if squats complete, go to 3x pulse squats 48 -> 50
          cleanUp();
          calib6b();
        } else if (exCounter == 51) {  // if 3 pulse squats complete, go to 3x jumps 51
          cleanUp();
          calib7();  // goes to endcalib, sets exCounter =0; cleanUp();
        }
      }
      break;
    case 0:  //reset
      {
        cleanUp();
        jumpCount = 0;
        exCounter = 0;  // reset the exercise counter
        mobScore = 0;   //reset the mobility score (calib4)
        strengthScore = 0; //reset the strength score (calib 5 and 6 a,b)
      }
      break;
    case 1:  // Entry GAME LEVEL at 80%
      {
        lineDelay = lineDelay1;  //line delay for entry level 1 at 80%
        liftOff = 0;             //wipeclean
        landing = 0;
        //jumpState = 0;
        cleanUp();
        playSet();
      }
      break;
    case 2:  //GAME LEVEL at 80% plus 5%
      {
        lineDelay = lineDelay2;  //line delay for level 2
        liftOff = 0;             //wipeclean
        landing = 0;
        jumpState = 0;
        cleanUp();
        playSet();
      }
      break;
    case 3:  //GAME LEVEL at 80% plus 10%
      {
        lineDelay = lineDelay3;  //line delay for level 3
        liftOff = 0;             //wipeclean
        landing = 0;
        jumpState = 0;
        cleanUp();
        playSet();
      }
      break;
    case 4:  //GAME LEVEL at 80% plus 15%
      {
        lineDelay = lineDelay4;  //line delay for level 4
        liftOff = 0;             //wipeclean
        landing = 0;
        jumpState = 0;
        cleanUp();
        playSet();
      }
      break;
    case 5:  //GAME LEVEL at 80% plus 20%
      {
        lineDelay = lineDelay5;  //line delay for level 5 = 100%
        liftOff = 0;             //wipeclean
        landing = 0;
        jumpState = 0;
        cleanUp();
        playSet();
      }
      break;
  }  //end of switch

  // after each jump, prep data for sending jump results over to button
  //myJump.t = airtime;
  //myJump.b = buttonInput;
  myJump.d = jumpCount;  // problem on last jump (6):  jump count is already set back to 0, so shows as 5? or not at all?
  myJump.e = jumpState;

  //prep data for sending jump results over to yellobyte ESP

  // Send message1 via ESP-NOW to button ESP
  esp_err_t result1 = esp_now_send(broadcastAddress1, (uint8_t *)&myJump, sizeof(myJump));
  if (result1 == ESP_OK) {
    //Serial.print(", myJump was sent with success");
  } else {
    //Serial.println("Error sending myJump");
  }

  // Send message2 via ESP-NOW to yellobyte
  //esp_err_t result2 = esp_now_send(broadcastAddress2, (uint8_t *)&mySound, sizeof(mySound));
  //if (result2 == ESP_OK && jumpState <= 6) {
  //Serial.print(", mySound was sent with success");
  //} else {
  //Serial.println("Error sending mySound");
  //}

  //updateSerial();
  //delay(interJump);  //Wait for next jump - remove as this affects calibration! <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

}  //end of loop


void cleanUp() {
  for (int i = 0; i < 16; i++) {  // wipe all tiles
    stripArray[i].clear();
    stripArray[i].show();
  }
}

//-------------------------------------START CALIBRATION-------------------------------------------------------

void calib1() {                // Calibration exercise 1: marching on the spot 60s total
  strip10.fill(left, 10, 11);  // LEFT FOOT is cyan
  strip10.fill(left, 52, 10);
  strip10.fill(left, 41, 11);
  strip10.show();
  delay(stepDelay);
  strip10.clear();
  strip10.show();

  strip11.fill(right, 20, 21);  // RIGHT FOOT is magenta
  strip11.fill(right, 51, 10);
  strip11.show();
  delay(stepDelay);
  strip11.clear();
  strip11.show();
  exCounter++;
}

void calib2() {                // exercise 2: balance on LEFT, RIGHT, TOES, HEELS 60s total
  strip10.fill(left, 10, 11);  // LEFT FOOT is cyan
  strip10.fill(left, 52, 10);
  strip10.fill(left, 41, 11);
  strip10.show();
  delay(balanceDelay * 2);  //LEFT STANDING BALANCE 20s
  strip10.clear();
  strip10.show();
  delay(stepDelay);  // switching  over small break

  strip11.fill(right, 20, 21);  // RIGHT FOOT is magenta
  strip11.fill(right, 51, 10);
  strip11.show();
  delay(balanceDelay * 2);  //RIGHT STANDING BALANCE 20s
  strip11.clear();
  strip11.show();
  delay(stepDelay);  // switching  over small break

  strip10.fill(left, 10, 11);  //LEFT toe on
  strip10.fill(left, 51, 5);
  strip10.fill(left, 5, 5);
  strip11.fill(right, 20, 11);  // RIGHT toe on
  strip11.fill(right, 51, 5);
  strip11.fill(right, 0, 5);
  strip10.show();
  strip11.show();
  delay(balanceDelay);  //10s toe balance
  strip10.clear();
  strip11.clear();
  strip10.show();
  strip11.show();
  delay(stepDelay);  // switching  over small break

  strip10.fill(left, 41, 10);  //LEFT heel on
  strip10.fill(left, 56, 6);
  strip10.fill(left, 5, 5);
  strip11.fill(right, 30, 11);  // RIGHT heel on
  strip11.fill(right, 56, 6);
  strip11.fill(right, 0, 5);
  strip10.show();
  strip11.show();
  delay(balanceDelay);  //10s heel balance
  strip10.clear();
  strip11.clear();
  strip10.show();
  strip11.show();
  exCounter++;
  delay(1000);  // switching  over small break
}

void calib3() {                 // exercise3: side lift 5xL and 5xR, 30s total
  strip11.fill(right, 20, 21);  // stand on right
  strip11.fill(right, 51, 10);
  strip10.fill(left, 0, 10);
  strip9.fill(left, 0, 10);
  strip11.show();
  strip10.show();
  strip9.show();
  delay(sideliftDelay);  // time to lift out left leg
  strip11.clear();
  strip10.clear();
  strip9.clear();
  strip11.show();
  strip10.show();
  strip9.show();
  delay(stepDelay);  // switching  over small break

  strip10.fill(left, 10, 11);  // stand on left
  strip10.fill(left, 52, 10);
  strip10.fill(left, 41, 11);
  strip11.fill(right, 0, 10);
  strip12.fill(right, 0, 10);
  strip10.show();
  strip11.show();
  strip12.show();
  delay(sideliftDelay);  // time to lift out right leg
  strip10.clear();
  strip11.clear();
  strip12.clear();
  strip10.show();
  strip11.show();
  strip12.show();
  delay(stepDelay);  // switching  over small break
  exCounter++;
  if (exCounter == 37) { //last one
    delay(1000);
  } else {
  }
}

void stepbackleft() {
  strip10.fill(left, 10, 11);  // step back on left
  strip10.fill(left, 52, 10);
  strip10.fill(left, 41, 11);
  strip10.show();
  delay(stepDelay);  // stand
  strip10.clear();   // left light off
  strip10.show();
}

void stepbackright() {
  strip11.fill(right, 20, 21);  // step back on right
  strip11.fill(right, 51, 10);
  strip11.show();
  delay(stepDelay);  // stand
  strip11.clear();   // right light off
  strip11.show();
}

void calib4() {                // exercise 4:  CLOCKFACE, 4 times, total time 60s
  strip10.fill(left, 10, 11);  //  left on
  strip10.fill(left, 52, 10);
  strip10.fill(left, 41, 11);
  strip11.fill(right, 20, 21);  // right on
  strip11.fill(right, 51, 10);
  strip10.show();
  strip11.show();
  delay(stepDelay);  //stand on both feet
  strip10.clear();   // left light off, right stays on
  strip10.show();

  strip6.fill(left, 5, 5);  // CLOCKFACE, take left fwd step --- 1
  strip6.fill(left, 41, 10);
  strip6.fill(left, 56, 5);
  strip6.show();
  delay(stepDelay);  //tap time
  if (T6B > weightOn) { // is there a weight on the sensor?
  mobScore++;
  } else {
  }
  strip6.clear();  // left light off
  strip6.show();

  stepbackleft();

  strip6.fill(left, 0, 5);  // left diagonally fwd step --- 2
  strip6.fill(left, 30, 11);
  strip6.fill(left, 56, 5);
  strip6.show();
  delay(stepDelay);  //tap
  if (T6B > weightOn) { // is there a weight on the sensor?
  mobScore++;
  } else {
  }
  strip6.clear();    // left light off
  strip6.show();

  stepbackleft();

  strip5.fill(left, 5, 5);  // big diagonal step left --- 3
  strip5.fill(left, 41, 10);
  strip5.fill(left, 56, 5);
  strip5.show();
  delay(stepDelay);  //tap
  if (T5B > weightOn) { // is there a weight on the sensor?
  mobScore++;
  } else {
  }
  strip5.clear();    // left light off
  strip5.show();

  stepbackleft();

  strip9.fill(left, 10, 11);  // step big sideways on left --- 4
  strip9.fill(left, 52, 10);
  strip9.fill(left, 41, 11);
  strip9.show();
  delay(stepDelay);  //tap
  if (T6A > weightOn || T6B > weightOn) { // is there a weight on either sensor?
  mobScore++;
  } else {
  }
  strip9.clear();    // left light off
  strip9.show();

  stepbackleft();

  strip13.fill(left, 10, 11);  // step diagonally back on left --- 5
  strip13.fill(left, 51, 5);
  strip13.fill(left, 5, 5);
  strip13.show();
  delay(stepDelay);  //tap
  if (T13A > weightOn) { // is there a weight on the sensor?
  mobScore++;
  } else {
  }
  strip13.clear();   // left light off
  strip13.show();

  stepbackleft();

  strip14.fill(left, 20, 21);  // step backwards on left --- 6
  strip14.fill(left, 52, 10);
  strip14.show();
  delay(stepDelay);  //tap
  if (T14A > weightOn || T14B > weightOn) { // is there a weight on either sensor?
  mobScore++;
  } else {
  }
  strip14.clear();   // left light off
  strip14.show();

  stepbackleft();

  strip7.fill(left, 20, 21);  // big step front across on left for 2 counts --- 7
  strip7.fill(left, 51, 10);
  strip7.show();
  delay(stepDelay * 2);  //tap
  if (T7A > weightOn || T7B > weightOn) { // is there a weight on either sensor?
  mobScore++;
  } else {
  }
  strip7.clear();        // left light off
  strip7.show();

  strip10.fill(left, 10, 11);  // step back on left
  strip10.fill(left, 52, 10);
  strip10.fill(left, 41, 11);
  strip10.show();
  delay(stepDelay * 2);  // stand on both feet for 2 counts ------------------------------8 HALFWAY
  

  strip11.clear();  // right light off, left stays on for CLOCKFACE LEFT
  strip11.show();

  strip7.fill(right, 30, 11);  // --- right fwd step ---1
  strip7.fill(right, 56, 5);
  strip7.fill(right, 0, 5);
  strip7.show();
  delay(stepDelay);  //tap
  if (T7B > weightOn) { // is there a weight on the sensor?
  mobScore++;
  } else {
  }
  strip7.clear();    //right light off
  strip7.show();

  stepbackright();

  strip7.fill(right, 41, 10);  //  right diagonal step fwd ----2
  strip7.fill(right, 5, 5);
  strip7.fill(right, 56, 5);
  strip7.show();
  delay(stepDelay);  //tap
  if (T7B > weightOn) { // is there a weight on the sensor?
  mobScore++;
  } else {
  }
  strip7.clear();    //right light off
  strip7.show();

  stepbackright();

  strip8.fill(right, 30, 11);  //  big diagonal step right ----3
  strip8.fill(right, 56, 5);
  strip8.fill(right, 0, 5);
  strip8.show();
  delay(stepDelay);  //tap
  if (T8B > weightOn) { // is there a weight on the sensor?
  mobScore++;
  } else {
  }
  strip8.clear();    //right light off
  strip8.show();

  stepbackright();

  strip12.fill(right, 20, 22);  //  right big side step  ----4
  strip12.fill(right, 52, 10);
  strip12.show();
  delay(stepDelay);  //tap
  if (T12A > weightOn || T12B > weightOn) { // is there a weight on either sensor?
  mobScore++;
  } else {
  }
  strip12.clear();   //right light off
  strip12.show();

  stepbackright();

  strip16.fill(right, 20, 11);  //  right diagonal back step  ----5
  strip16.fill(right, 51, 5);
  strip16.fill(right, 0, 5);
  strip16.show();
  delay(stepDelay);  //tap
  if (T16A > weightOn) { // is there a weight on the sensor?
  mobScore++;
  } else {
  }
  strip16.clear();   //right light off
  strip16.show();

  stepbackright();

  strip15.fill(right, 41, 11);  //  right backward step  ----6
  strip15.fill(right, 10, 11);
  strip15.fill(right, 52, 10);
  strip15.show();
  delay(stepDelay);  //tap
  if (T15A > weightOn || T15B > weightOn) { // is there a weight on either sensor?
  mobScore++;
  } else {
  }
  strip15.clear();   //right light off
  strip15.show();

  stepbackright();

  strip6.fill(right, 10, 10);  // big step front across on right --- 7
  strip6.fill(right, 52, 10);
  strip6.fill(right, 41, 11);
  strip6.show();
  delay(stepDelay * 2);  //step
  if (T6A > weightOn || T6B > weightOn) { // is there a weight on either sensor?
  mobScore++;
  } else {
  }
  strip6.clear();        // right light off
  strip6.show();

  strip11.fill(right, 20, 21);  // step back on right
  strip11.fill(right, 51, 10);
  strip11.show();
  delay(stepDelay * 2);  // stand on both feet  ------------------------------8 CoMPlETE
  strip10.clear();       // left off
  strip11.clear();       // right off
  strip10.show();
  strip11.show();
  exCounter++;
  if (exCounter == 39) {// last one
    delay(1000);
    //send mobScore off to button ESP
  } else {
  }
}

void calib5() {                // exercise 5: alternating LUNGES, 5x left, 5x right, total time 30s
  strip7.fill(right, 20, 21);  // step forward on right
  strip7.fill(right, 51, 10);
  strip6.fill(left, 10, 10);  // step forward on left
  strip6.fill(left, 52, 10);
  strip6.fill(left, 41, 11);
  strip6.show();
  strip7.show();
  delay(stepDelay);  // standing on tiles both on

  strip6.clear();  //left off, right stays on
  strip6.show();
  strip10.fill(left, 52, 10);  //left line on
  strip14.fill(left, 52, 10);
  strip10.show();
  strip14.show();
  delay(lungeDelay);  // time to move left leg back lunge
  if (T7A > weightOn && T7B > weightOn) { // is there a weight on both right sensors?
  strengthScore++;
  } else {
  }
  strip10.clear();    //left line clear
  strip14.clear();
  strip10.show();
  strip14.show();

  strip6.fill(left, 10, 10);  // step fwd together on left
  strip6.fill(left, 52, 10);
  strip6.fill(left, 41, 11);
  strip6.show();     //stand on both feet
  delay(stepDelay);  // switching  over small break

  strip7.clear();  // right foot off, left stays on
  strip7.show();
  strip11.fill(right, 52, 10);  //line on right
  strip15.fill(right, 52, 10);
  strip11.show();
  strip15.show();
  delay(lungeDelay);  // time to move right leg back lunge
  if (T6A > weightOn && T6B > weightOn) { // is there a weight on both left sensors?
  strengthScore++;
  } else {
  }
  strip11.clear();    //right line clear
  strip15.clear();
  strip11.show();
  strip15.show();

  strip7.fill(right, 20, 21);  // step fwd together on right
  strip7.fill(right, 51, 10);
  strip7.show();  // stand on both feet, tile 6 and 7
  delay(stepDelay);
  strip6.clear();  //left foot off
  strip7.clear();  //right foot off
  strip6.show();
  strip7.show();
  exCounter++;
}

void calib6a() {               // exercises 6 a and b:  squat x 3 (10s), squat with pulses x 3 (20s), total time 30s
  strip10.fill(left, 10, 11);  // step back LEFT
  strip10.fill(left, 52, 10);
  strip10.fill(left, 41, 11);
  strip11.fill(right, 20, 21);  // step back right
  strip11.fill(right, 51, 10);
  strip10.show();
  strip11.show();
  delay(stepDelay * 2);  // delay for stepping back, standing, resting
  strip10.clear();
  strip11.clear();
  strip10.show();
  strip11.show();
  //delay(5); //blink

  strip10.fill(white, 10, 11);  // stand LEFT
  strip10.fill(white, 52, 10);
  strip10.fill(white, 41, 11);
  strip11.fill(white, 20, 21);  // stand right
  strip11.fill(white, 51, 10);
  strip10.show();
  strip11.show();
  delay(squat1Delay);  //now do the squat in time
  if (T6A > weightOn && T6B > weightOn && T7A > weightOn && T7B > weightOn) { // is there a weight on both left sensors and both right sensors?
  strengthScore++;
  } else {
  }
  strip10.clear();
  strip11.clear();
  strip10.show();
  strip11.show();  //both lights off
  exCounter++;
}

void calib6b() {               // exercises 6 a and b:  squat x 3 (10s), squat with pulses x 3 (20s), total time 30s
  strip10.fill(left, 10, 11);  // step back LEFT
  strip10.fill(left, 52, 10);
  strip10.fill(left, 41, 11);
  strip11.fill(right, 20, 21);  // step back right
  strip11.fill(right, 51, 10);
  strip10.show();
  strip11.show();
  delay(stepDelay * 2);  // delay for stepping back, standing, resting
  strip10.clear();
  strip11.clear();
  strip10.show();
  strip11.show();
  //delay(5);//blink

  strip10.fill(white, 10, 11);  // stand LEFT
  strip10.fill(white, 52, 10);
  strip10.fill(white, 41, 11);
  strip11.fill(white, 20, 21);  // stand right
  strip11.fill(white, 51, 10);
  strip10.show();
  strip11.show();
  delay(squat2Delay);  //now do the squat in 
  if (T6A > weightOn && T6B > weightOn && T7A > weightOn && T7B > weightOn) { // is there a weight on both left sensors and both right sensors?
  strengthScore++;
  } else {
  }
  strip10.clear();
  strip11.clear();
  strip10.show();
  strip11.show();  //both lights off
  exCounter++;
  if (exCounter == 50) {//squats complete
    delay(1000);
    //also send off strengthScore to Button ESP
  } else {
  }
}

void calib7() {  // exercise 7:  airtime x 3, total time for jumping 15s
  // 14 and 15 are thicker tiles for jumping
  strip14.fill(left, 10, 41);   // LEFT TiLE is cyan
  strip15.fill(right, 10, 41);  // RIGHT Tile is magenta
  strip14.show();
  strip15.show();
  delay(jumpDelay);  //time to do 3 airtime jumps
  strip14.clear();
  strip15.clear();
  strip14.show();
  strip15.show();
  exCounter++;

  delay(stepDelay);  //short delay
  calibend();

  delay(18000);  // a delay to do the manual switch from calib to RESET
  // end calibration, we return to void loop but need to receive manual reset now.
  //buttonInput = 0; // would be the reset command but could be overriden by web site button.
}

void calibend() {
  for (int i = 0; i < 16; i++) {        // flash all tiles at the end of the calibration cycle
    stripArray[i].fill(purple, 0, 61);  // all purple
    stripArray[i].show();
    delay(stepDelay / 8);
    stripArray[i].clear();
    stripArray[i].show();
  }
  exCounter = 0;  // reset the exercise counter
  strip1.fill(purple, 0, 61);
  strip2.fill(purple, 0, 61);
  strip3.fill(purple, 0, 61);
  strip4.fill(purple, 0, 61);
  strip5.fill(purple, 0, 61);
  strip6.fill(purple, 0, 61);
  strip7.fill(purple, 0, 61);
  strip8.fill(purple, 0, 61);
  strip9.fill(purple, 0, 61);
  strip10.fill(purple, 0, 61);
  strip11.fill(purple, 0, 61);
  strip12.fill(purple, 0, 61);
  strip13.fill(purple, 0, 61);
  strip14.fill(purple, 0, 61);
  strip15.fill(purple, 0, 61);
  strip16.fill(purple, 0, 61);
  strip1.show();
  strip2.show();
  strip3.show();
  strip4.show();
  strip5.show();
  strip6.show();
  strip7.show();
  strip8.show();
  strip9.show();
  strip10.show();
  strip11.show();
  strip12.show();
  strip13.show();
  strip14.show();
  strip15.show();
  strip16.show();
  delay(stepDelay * 2);
  cleanUp();
}

//----------------------------------------START PLAY----------------------------------------------------
void goodJump() {
  strip1.fill(green, 0, 61);
  strip2.fill(green, 0, 61);
  strip3.fill(green, 0, 61);
  strip4.fill(green, 0, 61);
  strip1.show();
  strip2.show();
  strip3.show();
  strip4.show();
  delay(resultDelay * 1.5);
  // strip1.clear();
  // strip2.clear();
  // strip3.clear();
  // strip4.clear();
  strip1.fill(black, 0, 61);
  strip2.fill(black, 0, 61);
  strip3.fill(black, 0, 61);
  strip4.fill(black, 0, 61);
  strip1.show();
  strip2.show();
  strip3.show();
  strip4.show();
  delay(resultDelay / 2);
}

void partialJump() {
  strip1.fill(yellow, 0, 61);
  strip2.fill(yellow, 0, 61);
  strip3.fill(yellow, 0, 61);
  strip4.fill(yellow, 0, 61);
  strip1.show();
  strip2.show();
  strip3.show();
  strip4.show();
  delay(resultDelay);
  strip1.clear();
  strip2.clear();
  strip3.clear();
  strip4.clear();
  strip1.show();
  strip2.show();
  strip3.show();
  strip4.show();
}

void badJump() {
  strip1.fill(red, 0, 61);
  strip2.fill(red, 0, 61);
  strip3.fill(red, 0, 61);
  strip4.fill(red, 0, 61);
  strip1.show();
  strip2.show();
  strip3.show();
  strip4.show();
  delay(resultDelay);
  strip1.clear();
  strip2.clear();
  strip3.clear();
  strip4.clear();
  strip1.show();
  strip2.show();
  strip3.show();
  strip4.show();
}

void endJump() {
  strip1.fill(white, 10, 41);
  strip2.fill(white, 10, 41);
  strip3.fill(white, 10, 41);
  strip4.fill(white, 10, 41);
  strip1.show();
  strip2.show();
  strip3.show();
  strip4.show();
  delay(resultDelay * 20);
  strip1.clear();
  strip2.clear();
  strip3.clear();
  strip4.clear();
  strip1.show();
  strip2.show();
  strip3.show();
  strip4.show();
}

void playSet() {
  uint32_t levColor = strip10.Color(255, 255, 255);  //white

  switch (buttonInput) {
    case 1:
      {
        levColor = lime;
      }
      break;
    case 2:
      {
        levColor = cyan;
      }
      break;
    case 3:
      {
        levColor = blue;
      }
      break;
    case 4:
      {
        levColor = purple;
      }
      break;
    case 5:
      {
        levColor = magenta;
      }
      break;
  }

  //maybe put interjump delay here????
  delay(interJump);  //Wait for next jump

  //ROW 1 LINE 1: set lights of strips to level  color
  strip1.fill(levColor, 15, 11);
  strip2.fill(levColor, 15, 11);
  strip3.fill(levColor, 15, 11);
  strip4.fill(levColor, 15, 11);
  strip1.show();
  strip2.show();
  strip3.show();
  strip4.show();
  delay(lineDelay);  //wait
  strip1.clear();
  strip2.clear();
  strip3.clear();
  strip4.clear();

  //ROW 1 LINE 2: set lights of strips to level color
  strip1.fill(levColor, 0, 10);
  strip2.fill(levColor, 0, 10);
  strip3.fill(levColor, 0, 10);
  strip4.fill(levColor, 0, 10);
  strip1.show();
  strip2.show();
  strip3.show();
  strip4.show();
  delay(lineDelay);  //wait
  strip1.clear();
  strip2.clear();
  strip3.clear();
  strip4.clear();

  //ROW 1 LINE 3: set lights of strips to level color
  strip1.fill(levColor, 36, 10);
  strip2.fill(levColor, 36, 10);
  strip3.fill(levColor, 36, 10);
  strip4.fill(levColor, 36, 10);
  strip1.show();
  strip2.show();
  strip3.show();
  strip4.show();
  delay(lineDelay);  //wait
  strip1.clear();
  strip2.clear();
  strip3.clear();
  strip4.clear();

  strip1.show();
  strip2.show();
  strip3.show();
  strip4.show();  //clear again??

  //ROW 2 LINE 1: set lights of strips to level color
  strip5.fill(levColor, 15, 11);
  strip6.fill(levColor, 15, 11);
  strip7.fill(levColor, 15, 11);
  strip8.fill(levColor, 15, 11);
  strip5.show();
  strip6.show();
  strip7.show();
  strip8.show();
  delay(lineDelay);  //wait
  strip5.clear();
  strip6.clear();
  strip7.clear();
  strip8.clear();

  //ROW 2 LINE 2: set lights of strips to level color
  strip5.fill(levColor, 0, 10);
  strip6.fill(levColor, 0, 10);
  strip7.fill(levColor, 0, 10);
  strip8.fill(levColor, 0, 10);
  strip5.show();
  strip6.show();
  strip7.show();
  strip8.show();
  delay(lineDelay);  //wait
  strip5.clear();
  strip6.clear();
  strip7.clear();
  strip8.clear();

  //ROW 2 LINE 3: set lights of strips to level color
  strip5.fill(levColor, 36, 10);
  strip6.fill(levColor, 36, 10);
  strip7.fill(levColor, 36, 10);
  strip8.fill(levColor, 36, 10);
  strip5.show();
  strip6.show();
  strip7.show();
  strip8.show();
  delay(lineDelay);  //wait
  strip5.clear();
  strip6.clear();
  strip7.clear();
  strip8.clear();

  strip5.show();
  strip6.show();
  strip7.show();
  strip8.show();  //clear again??

  //ROW 3 LINE 1: set lights of strips to level color
  strip9.fill(levColor, 15, 11);
  strip10.fill(levColor, 15, 11);
  strip11.fill(levColor, 15, 11);
  strip12.fill(levColor, 15, 11);
  strip9.show();
  strip10.show();
  strip11.show();
  strip12.show();
  delay(lineDelay);  //wait
  strip9.clear();
  strip10.clear();
  strip11.clear();
  strip12.clear();

  //ROW 3 LINE 2: set lights of strips to level color
  strip9.fill(levColor, 0, 10);
  strip10.fill(levColor, 0, 10);
  strip11.fill(levColor, 0, 10);
  strip12.fill(levColor, 0, 10);
  strip9.show();
  strip10.show();
  strip11.show();
  strip12.show();
  delay(lineDelay);  //wait
  strip9.clear();
  strip10.clear();
  strip11.clear();
  strip12.clear();

  //ROW 3 LINE 3: set lights of strips to level color
  strip9.fill(levColor, 36, 10);
  strip10.fill(levColor, 36, 10);
  strip11.fill(levColor, 36, 10);
  strip12.fill(levColor, 36, 10);
  strip9.show();
  strip10.show();
  strip11.show();
  strip12.show();
  delay(lineDelay);  //wait
  strip9.clear();
  strip10.clear();
  strip11.clear();
  strip12.clear();

  strip9.show();
  strip10.show();
  strip11.show();
  strip12.show();  //clear again??

  //ROW 4 LINE 1: set lights of strips to level color, check for liftoff
  strip13.fill(levColor, 15, 11);
  strip14.fill(levColor, 15, 11);
  strip15.fill(levColor, 15, 11);
  strip16.fill(levColor, 15, 11);
  strip13.show();
  strip14.show();

  strip15.show();
  strip16.show();
  delay(lineDelay);  //wait
  strip13.clear();
  strip14.clear();
  strip15.clear();
  strip16.clear();

  //ROW 4 LINE 2: set lights of strips to level color
  strip13.fill(levColor, 0, 10);
  strip14.fill(levColor, 0, 10);
  strip15.fill(levColor, 0, 10);
  strip16.fill(levColor, 0, 10);
  strip13.show();
  strip14.show();

  leftSensor = myData.d;
  Serial.print(", Left tile: ");
  Serial.print(leftSensor);  //checking
  if (leftSensor > 120) {    // person on tile 14?
    liftOff = 1;             //was too slow to jump off
  } else {
    liftOff = 0;
  }
  strip15.show();
  strip16.show();
  delay(lineDelay);  //wait
  strip13.clear();
  strip14.clear();
  strip15.clear();
  strip16.clear();

  //ROW 4 LINE 3: set lights of strips to level color, check landing
  strip13.fill(levColor, 36, 10);
  strip14.fill(levColor, 36, 10);
  strip15.fill(levColor, 36, 10);
  strip16.fill(levColor, 36, 10);
  strip13.show();
  strip14.show();
  strip15.show();
  rightSensor = myData.e;
  Serial.print(", Right tile: ");
  Serial.print(rightSensor);  //checking
  if (rightSensor > 120) {    // person is on tile 15
    landing = 1;              // was landing too early
  } else {
    landing = 0;
  }
  strip16.show();
  delay(lineDelay);  //wait
  strip13.clear();
  strip14.clear();
  strip15.clear();
  strip16.clear();

  strip13.show();
  strip14.show();
  strip15.show();
  strip16.show();  // clear again?
  //delay(lineDelay);//why wait again?

  jumpState = (liftOff + landing + 1);
  //Serial.print(", jump was... ");  // print out the values you read:
  //Serial.println(jumpState);       // one more jump completed

  jumpCount++;
  // Serial.print(", jumpCount: ");  // print out the values you read:
  // Serial.print(jumpCount);

  mySound.d = jumpCount;  // use jump 6 for endsound. but jump count is already set back to 0, so shows as 5? or not at all?
  //mySound.b = endJump? 0,1
  mySound.e = jumpState;  // 0,1,2,3 - nothing, fail, partial, good

  // Send message2 via ESP-NOW to yellobyte
  esp_err_t result2 = esp_now_send(broadcastAddress2, (uint8_t *)&mySound, sizeof(mySound));
  if (result2 == ESP_OK) {
    //Serial.print(", mySound was sent with success");
  } else {
    //Serial.println("Error sending mySound");
  }
  
  delay(800);

  if (jumpState == 1) {  //1 = successful jump
    goodJump();
  } else if (jumpState == 2) {  // 2= lift off too late or land too early
    partialJump();
  } else if (jumpState == 3) {  //3 = bad jump
    badJump();                 // was stuck on tile
  } else {                     // 0 = jumpState
  }

  if (jumpCount > 5) {  // after 6 jumps start at 0
    endJump();
    jumpCount = 0;
    //delay(lineDelay * 10);  // time for manual press of reset button
    //buttonInput = 0; //set to 0 after 6 jumps - but not working if button is still pressed -  needs manual press!
  }

  //end of void playSet, jump is complete, we return to void loop to send jumpState off
}

//--------------------------------------------------------------------------------------------------------------------
void updateSerial() {
  //display incoming data in serial monitor
  Serial.print("gamelevel; ");
  Serial.println(myData.b);
  Serial.print("T14; ");  // print out the values you read:
  Serial.println(myData.d);
  Serial.print("T15; ");  // print out the values you read:
  Serial.println(myData.e);
  Serial.print("lineDelay; ");  // print out the values you read:
  Serial.println(lineDelay);
  Serial.print("lineDelay1; ");  // print out the values you read:
  Serial.println(lineDelay1);
  Serial.print("lineDelay2; ");  // print out the values you read:
  Serial.println(lineDelay2);
  Serial.print("lineDelay3; ");  // print out the values you read:
  Serial.println(lineDelay3);
  Serial.print("lineDelay4; ");  // print out the values you read:
  Serial.println(lineDelay4);
  Serial.print("lineDelay5; ");  // print out the values you read:
  Serial.println(lineDelay5);
  //  delay(10);
}

//---------------------------------------FUNCTIONALITY-------------------------------------------------//
void updateTiles()
{
  for(int i = 0; i < 16; i++)
  {
    tiles[i].Strip().show();
  }
}
void clearTiles()
{
    for(int i = 0; i < 16; i++)
  {
    tiles[i].Strip().clear();
  }
}
