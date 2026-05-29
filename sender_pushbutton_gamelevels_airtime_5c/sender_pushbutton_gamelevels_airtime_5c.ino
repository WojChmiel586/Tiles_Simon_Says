// set pin numbers for game levels and control_light keypad
const int resetPin = 10;   // Button pin reset
//const int calibPin = 9;   // Button pin calibration replaced by section below
const int entryPin = 8;   // Button pin 80%
const int fivePin = 7;     // Button pin 85%
const int tenPin = 6;      // Button pin 90%
const int fifteenPin = 5;  // Button pin 95%
// const int twentyPin = 4;  // Button pin 100% - PIN used for num pad LED
const int RedPin = 4;     // Led pin for feedback on 3 digit number entered keypad
const int calib1 = 9; 
const int calib2 = 1;
const int calib3 = 2;
const int calib4 = 45;
const int calib5 = 48;
const int calib6 = 47;
const int calib7 = 21;
const int calibend = 46; //log

// variables for storing the game level and calib button status
int resetbuttonState = 0;
int calibbuttonState = 0;
int entrybuttonState = 0;
int fivebuttonState = 0;
int tenbuttonState = 0;
int fifteenbuttonState = 0;
//int twentybuttonState = 0;
int calib1State = 0;
int calib2State = 0;
int calib3State = 0;
int calib4State = 0;
int calib5State = 0;
int calib6State = 0;
int calib7State = 0;
int calibendState = 0;
int gamelevel = 0;
int previousGameLevel = -1;
String serialBuffer = "";

#include <Keypad.h>
const byte ROWS = 4;  //four rows
const byte COLS = 4;  //four columns
//define the cymbols on the buttons of the keypads
char hexaKeys[ROWS][COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};
byte rowPins[ROWS] = { 42, 41, 40, 39 };  //connect to the row pinouts of the keypad LtoR
byte colPins[COLS] = { 38, 37, 36, 35 };  //connect to the column pinouts of the keypad LtoR
//initialize an instance of class NewKeypad
Keypad MyKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

const int len_key = 3;      // we need to enter 3 digits in the keypad
char attempt_key[len_key];  // we set up a 3 digit input array for keypad use
int airtime = 550;           // default
int z = 0;                  // wipes keypad numbers clean

#include <esp_now.h>
#include <WiFi.h>
uint8_t broadcastAddress[] = { 0xEC, 0xDA, 0x3B, 0x95, 0xC5, 0x0C };  //ESP address = receiver is game ESP ec:da:3b:95:c5:0c
//uint8_t broadcastAddress[] = { 0x80, 0x65, 0x99, 0xC8, 0xB9, 0xFC };  //ESP address = receiver is upper sensor ESP

String success;

typedef struct struct_message_all { // sender/receiver must match structure
  int id; // unique sender ID: LO = 1, LI = 2, RI = 3, RO = 4, button ESP = 5, game ESP = 6
  int t;  // can be used for airtime
  int b;  // can be used for buttonInput
  int jc; // can be used for jumpCount 
  int js; // can be used for jumpState
  int sd; // can be used for stepDelay
  int dA; // toe sensor (A) of the first tile
  int dB;
  int eA;
  int eB;
  int fA;
  int fB;
  int gA;
  int gB;
} struct_message_all;

struct_message_all myLevel;  // name of outgoing struct_message is myLevel
//struct_message_short myJump;  // name of incoming message

esp_now_peer_info_t peerInfo;
// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  //Serial.print("\r\nLast Packet Send Status:\t");
  //Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  //if (status == 0) {
    //success = "Delivery Success";
  //} else {
    //success = "Delivery Fail";
  //}
}

/*void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  memcpy(&myJump, incomingData, sizeof(myJump));
  //airtimecheck = myJump.t;
  //gamelevelcheck = myJump.b;
  jumpCount = myJump.d;
  jumpState = myJump.e;
}*/

void setup() {
  Serial.begin(115200);
  pinMode(resetPin, INPUT);    // initialize the button pin as an input
  //pinMode(calibPin, INPUT);    // initialize the button pin as an input
  pinMode(entryPin, INPUT);    // initialize the button pin as an input
  pinMode(fivePin, INPUT);     // initialize the button pin as an input
  pinMode(tenPin, INPUT);      // initialize the button pin as an input
  pinMode(fifteenPin, INPUT);  // initialize the button pin as an input
  //pinMode(twentyPin, INPUT);   // initialize the button pin as an input
  pinMode(calib1, INPUT);   // initialize the button pin as an input
  pinMode(calib2, INPUT);   // initialize the button pin as an input
  pinMode(calib3, INPUT);   // initialize the button pin as an input
  pinMode(calib4, INPUT);   // initialize the button pin as an input
  pinMode(calib5, INPUT);   // initialize the button pin as an input
  pinMode(calib6, INPUT);   // initialize the button pin as an input
  pinMode(calib7, INPUT);   // initialize the button pin as an input
  pinMode(calibend, INPUT);   // initialize the button pin as an input       
  pinMode(RedPin, OUTPUT);     // initialize the keypad Led pin as output

  WiFi.mode(WIFI_STA);
  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    //Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_send_cb(OnDataSent);

  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    //Serial.println("Failed to add peer");
    return;
  }

  // esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
}

void loop() {
  char key = MyKeypad.getKey();// capture the key input
  if (key) {
    switch (key) {
      case '*': // if * is pressed we start a new 3-digit number array
        z = 0; //by wiping numbers clean
        //delay(100);  
        break;
      case '#': //if this is pressed we check the array 
        //delay(100);                  // after an added debounce??
        //Serial.println(attempt_key); // we see the entered 3-number array displayed
        checkKEY();// check if it is a 3 digit array, if yes, enter into airtime
        sendOffAir(); // we send airtime on to game ESP
        break;
      default:
        //Serial.println(key);// we see all keys being pressed
        attempt_key[z] = key; // we enter a digit to the attempt key
        z++; //we count up
    }
  }

  // // read the state of all button values
  // resetbuttonState = digitalRead(resetPin);
  // //calibbuttonState = digitalRead(calibPin);
  // entrybuttonState = digitalRead(entryPin);
  // fivebuttonState = digitalRead(fivePin);
  // tenbuttonState = digitalRead(tenPin);
  // fifteenbuttonState = digitalRead(fifteenPin);
  // //twentybuttonState = digitalRead(twentyPin);
  // calib1State = digitalRead(calib1); 
  // calib2State = digitalRead(calib2);
  // calib3State = digitalRead(calib3);
  // calib4State = digitalRead(calib4);
  // calib5State = digitalRead(calib5);
  // calib6State = digitalRead(calib6);
  // calib7State = digitalRead(calib7);
  // calibendState = digitalRead(calibend);
  // //delay(5);

  // if (resetbuttonState == HIGH &&  previousGameLevel != 0) {  // if the reset button is pressed
  //   gamelevel = 0; 
  //   previousGameLevel = gamelevel;
  //   sendOff();
  // } else {
  // }
  // if (calib1State == HIGH && previousGameLevel != 91) {  // if the calibration button is pressed
  //   gamelevel = 91;
  //   previousGameLevel = gamelevel;
  //   sendOff();
  // } else {
  // }
  // if (calib2State == HIGH && previousGameLevel != 92) {  // if the calibration button is pressed
  //   gamelevel = 92;
  //   previousGameLevel = gamelevel;
  //   sendOff();
  // } else {
  // }
  // if (calib3State == HIGH && previousGameLevel != 93) {  // if the calibration button is pressed
  //   gamelevel = 93;
  //   previousGameLevel = gamelevel;
  //   sendOff();
  // } else {
  // }
  // if (calib4State == HIGH && previousGameLevel != 94) {  // if the calibration button is pressed
  //   gamelevel = 94;
  //   previousGameLevel = gamelevel;
  //   sendOff();
  // } else {
  // }
  // if (calib5State == HIGH && previousGameLevel != 95) {  // if the calibration button is pressed
  //   gamelevel = 95;
  //   previousGameLevel = gamelevel;
  //   sendOff();
  // } else {
  // }
  // if (calib6State == HIGH && previousGameLevel != 96) {  // if the calibration button is pressed
  //   gamelevel = 96;
  //   previousGameLevel = gamelevel;
  //   sendOff();
  // } else {
  // }
  // if (calib7State == HIGH && previousGameLevel != 97) {  // if the calibration button is pressed
  //   gamelevel = 97;
  //   previousGameLevel = gamelevel;
  //   sendOff();
  // } else {
  // }
  // if (calibendState == HIGH && previousGameLevel != 98) {  // if the calibration button is pressed
  //   gamelevel = 98;
  //   previousGameLevel = gamelevel;
  //   sendOff();
  // } else {
  // }
  // if (entrybuttonState == HIGH && previousGameLevel != 1) {  // if the button is pressed
  //   gamelevel = 1;
  //   previousGameLevel = gamelevel;
  //   sendOff();
  // } else {
  // }
  // if (fivebuttonState == HIGH && previousGameLevel != 2) {  // if the button is pressed
  //   gamelevel = 2;
  //   previousGameLevel = gamelevel;
  //   sendOff();
  // } else {
  // }
  // if (tenbuttonState == HIGH && previousGameLevel != 3) {  // if the button is pressed
  //   gamelevel = 3;
  //   previousGameLevel = gamelevel;
  //   sendOff();
  // } else {
  // }
  // if (fifteenbuttonState == HIGH && previousGameLevel != 4) {  // if the button is pressed
  //   gamelevel = 4;
  //   previousGameLevel = gamelevel;
  //   sendOff();
  // } else {
  // }
/*  if (twentybuttonState == HIGH) {  // if the button is pressed
    gamelevel = 5;
    sendOff();
  } else {
  } */

  // Read serial input -> send number on to game ESP to play a game. once done, we play sound.
  // we need to input 91, 92, 93, 95, 97, or 98 to start a game. Do not send 0 to reset
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (serialBuffer.length() > 0) {
        processSerialCommand(serialBuffer);// this is where we process the number
        serialBuffer = "";
      }
    } else {
      serialBuffer += c;
    }
  }


  //Serial.print("key; "); // we see the numbers being entered
  //Serial.println(attempt_key); // we see the entered array displayed - still not a number!
  /*clockTime = millis(); //define unit of time
  Serial.print("Time: ");
  Serial.print(clockTime);*/
  // Serial.print("ID: "); // we see the entered number displayed
  // Serial.print(myLevel.id); // we see the entered number displayed
  // Serial.print(", Airtime: "); // we see the entered number displayed
  // Serial.print(airtime); // we see the entered number displayed
  // Serial.print(", Game level: ");  // print current level
  // Serial.println(gamelevel);     // print current level
  // delay(10);
} // end of loop
//==============================================
void processSerialCommand(String command) 
{
  command.trim();

  if (command.length() == 0) return;

  // Check if it's the list command
  // if (command.equalsIgnoreCase("list")) 
  // {
  //   listAllFiles();
  //   return;
  // }

  // Check if it's a number
  bool isNumber = true;
  for (unsigned int i = 0; i < command.length(); i++) 
  {
    if (!isDigit(command.charAt(i))) {
      isNumber = false;
      break;
    }
  }

  int fileIndex = -1;  //variable to hold the data entered via serial monitor. cant use 0 as this is assigned

  if (isNumber) 
  {
    // It's a number, try to parse it
    fileIndex = command.toInt();  // set the fileIndex to the int number identified?

    Serial.print("Number entered is: ");
    Serial.println(fileIndex);  //print the number
    //Send a message with the number, even if it is out of range
    //myGame.b = fileIndex;  // send this number
    //esp_now_send(receiverAddress, (uint8_t *)&myGame, sizeof(myGame));
    //delay(500);

    /*if (fileIndex < 0 || fileIndex >= totalAudioFiles) {  // these are not within range: 91, 92, 93, 94, 95, 96, 97, 98
      //Serial.printf("Invalid file number. Please enter 0-%d\n", totalAudioFiles - 1); // do nothing
      return;
    }*/

  } /*else {
    // It's a name, search for it
    for (int i = 0; i < totalAudioFiles; i++) {
      if (audioFiles[i].name.equalsIgnoreCase(command)) {
        fileIndex = i;
        break;
      }
    }*/

    if (fileIndex < 0) 
    {
      Serial.printf("File '%s' not found. Type 'list' to see all files.\n", command.c_str());
      return;
    }

  // Go and play the file 
  //playAudioFile(fileIndex);
  gamelevel = fileIndex;
  sendOff();
}//end of process Input


void sendOff() {
  // Prep values to send
  myLevel.id = 5; // id of button ESP
  //myLevel.t = airtime;
  myLevel.b = gamelevel;
  //myLevel.sd = 500; //step delay default
  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&myLevel, sizeof(myLevel));

  Serial.print("ID: "); // we see the entered number displayed
  Serial.print(myLevel.id); // we see the entered number displayed
  Serial.print(", Airtime: "); // we see the entered number displayed
  Serial.print(airtime); // we see the entered number displayed
  Serial.print(", Game level: ");  // print current level
  Serial.println(gamelevel);     // print current level
  //if (result == ESP_OK) {
    //Serial.println("Sent with success");
  //} else {
    //Serial.println("Error sending the data");
  //}
}

void sendOffAir() {
  // Prep values to send
  myLevel.id = 5; // id of button ESP
  myLevel.t = airtime;
  //myLevel.b = gamelevel;
  //myLevel.sd = 500; //step delay default
  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&myLevel, sizeof(myLevel));
  //if (result == ESP_OK) {
    //Serial.println("Sent with success");
  //} else {
    //Serial.println("Error sending the data");
  //}
}

void checkKEY() { //this checks if the keypad number is a 3 digit array
  if (z == len_key) { 
    digitalWrite(RedPin, HIGH);  // turn LED on
    delay(200);
    digitalWrite(RedPin, LOW);  // turn LED off
    //Serial.println(attempt_key); // we see the entered array displayed - still not a number!
    airtime = atoi(attempt_key); // array converted to number to send on 
    z = 0; //wipe attempt_key clean and start over
  } else { // not a 3 digit number. Error after 4+ digits entered!!!
    delay(100);
    z = 0; // wipe clean and start over
  }
  for (int zz = 0; zz < len_key; zz++) {
    attempt_key[zz] = 0; // resets all 3 keys entered so numbers cannot be re-submitted
  }
}