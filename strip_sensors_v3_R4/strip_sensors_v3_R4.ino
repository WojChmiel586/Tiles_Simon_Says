#include <esp_now.h>
#include <WiFi.h>

uint8_t broadcastAddress2[] = { 0xEC, 0xDA, 0x3B, 0x95, 0xC5, 0x0C };  // game ESP
uint8_t broadcastAddress3[] = { 0x24, 0xEC, 0x4A, 0x00, 0x92, 0xF8 };  // data ESP

int T13A, T13B, T14A, T14B, T15A, T15B, T16A, T16B;

unsigned long lastSend = 0;
const unsigned long SEND_INTERVAL = 50;  // Increased from 20ms

// Track callbacks for BOTH peers
volatile int callbacksReceived = 0;
volatile int callbacksExpected = 0;

typedef struct struct_message_all {
  int id;
  int t, b, jc, js, sd;
  int dA, dB, eA, eB, fA, fB, gA, gB;
} struct_message_all;

struct_message_all mySensors;
esp_now_peer_info_t peerInfo;

// Callback tracks how many sends completed
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  callbacksReceived++;
  
  if (status != ESP_NOW_SEND_SUCCESS) {
    Serial.print("Send failed to: ");
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
             mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    Serial.println(macStr);
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }
  
  esp_now_register_send_cb(OnDataSent);

  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Add peer 2 - game ESP
  memcpy(peerInfo.peer_addr, broadcastAddress2, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add game ESP");
    return;
  }
  
  // Add peer 3 - data ESP
  memcpy(peerInfo.peer_addr, broadcastAddress3, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add data ESP");
    return;
  }
  
  // Stagger based on ID to avoid collisions
  int myId = 4;  // Change for each ESP: 1, 2, 3, 4
  delay(myId * 12);
  lastSend = millis();
  
  Serial.print("ESP ID ");
  Serial.print(myId);
  Serial.println(" ready");
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Read sensors
  T13A = analogRead(4);
  T13B = analogRead(5);
  T14A = analogRead(6);
  T14B = analogRead(7);
  T15A = analogRead(1);
  T15B = analogRead(2);
  T16A = analogRead(9);
  T16B = analogRead(10);
  
  // Prepare data
  mySensors.id = 4;
  mySensors.dA = T13A;
  mySensors.dB = T13B;
  mySensors.eA = T14A;
  mySensors.eB = T14B;
  mySensors.fA = T15A;
  mySensors.fB = T15B;
  mySensors.gA = T16A;
  mySensors.gB = T16B;

  // Send when: (1) interval elapsed AND (2) all previous callbacks received
  if ((currentMillis - lastSend >= SEND_INTERVAL) && 
      (callbacksReceived >= callbacksExpected))
  {
    lastSend = currentMillis;
    callbacksExpected += 2;  // Expecting 2 callbacks (one per peer)
    
    // Send to data ESP first
    esp_err_t result1 = esp_now_send(broadcastAddress3, (uint8_t *)&mySensors, sizeof(mySensors));
    if (result1 != ESP_OK) {
      Serial.print("Data ESP send error: ");
      Serial.println(result1);
      callbacksExpected--;  // Won't get callback for failed send
    }
    
    // Small delay between sends to avoid queue overflow
    delayMicroseconds(500);
    
    // Send to game ESP
    esp_err_t result2 = esp_now_send(broadcastAddress2, (uint8_t *)&mySensors, sizeof(mySensors));
    if (result2 != ESP_OK) {
      Serial.print("Game ESP send error: ");
      Serial.println(result2);
      callbacksExpected--;  // Won't get callback for failed send
    }
  }
  
  yield();  // Critical for WiFi stack!
}