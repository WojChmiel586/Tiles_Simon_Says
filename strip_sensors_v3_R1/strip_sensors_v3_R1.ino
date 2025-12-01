#include <esp_now.h>
#include <WiFi.h>

uint8_t broadcastAddress2[] = { 0xEC, 0xDA, 0x3B, 0x95, 0xC5, 0x0C };  // game ESP
uint8_t broadcastAddress3[] = { 0x24, 0xEC, 0x4A, 0x00, 0x92, 0xF8 };  // data ESP

int T1A, T1B, T2A, T2B, T3A, T3B, T4A, T4B;

unsigned long lastSend = 0;
const unsigned long SEND_INTERVAL = 50;  // Interval between sending packets

// Track callbacks for both peers
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
  
  if (status != ESP_NOW_SEND_SUCCESS) 
  {
    Serial.print("Send failed to: ");
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
             mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    Serial.println(macStr);
  }
}

void setup() 
{
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  
  if (esp_now_init() != ESP_OK) 
  {
    Serial.println("ESP-NOW init failed");
    return;
  }
  
  esp_now_register_send_cb(OnDataSent);

  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Add peer 2 - game ESP
  memcpy(peerInfo.peer_addr, broadcastAddress2, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK) 
  {
    Serial.println("Failed to add game ESP");
    return;
  }
  
  // Add peer 3 - data ESP
  memcpy(peerInfo.peer_addr, broadcastAddress3, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("Failed to add data ESP");
    return;
  }
  
  // Stagger based on ID to avoid collisions
  int myId = 1;  // Change for each ESP: 1, 2, 3, 4
  delay(myId * 12);
  lastSend = millis();
  
  Serial.print("ESP ID ");
  Serial.print(myId);
  Serial.println(" ready");
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Read sensors
  T1A = analogRead(4);
  T1B = analogRead(5);
  T2A = analogRead(6);
  T2B = analogRead(7);
  T3A = analogRead(1);
  T3B = analogRead(2);
  T4A = analogRead(9);
  T4B = analogRead(10);
  
  // Prepare data
  mySensors.id = 1;
  mySensors.dA = T1A;
  mySensors.dB = T1B;
  mySensors.eA = T2A;
  mySensors.eB = T2B;
  mySensors.fA = T3A;
  mySensors.fB = T3B;
  mySensors.gA = T4A;
  mySensors.gB = T4B;

  // Send when interval elapsed AND all previous callbacks received
  if ((currentMillis - lastSend >= SEND_INTERVAL) && 
      (callbacksReceived >= callbacksExpected))
  {
    lastSend = currentMillis;
    callbacksExpected += 2;  // Expecting 2 callbacks (one per peer)
    
    // Send to data ESP first
    esp_err_t result1 = esp_now_send(broadcastAddress3, (uint8_t *)&mySensors, sizeof(mySensors));
    if (result1 != ESP_OK) 
    {
      Serial.print("Data ESP send error: ");
      Serial.println(result1);
      callbacksExpected--;  // Won't get callback for failed send
    }
    
    // Small delay between sends to avoid queue overflow
    delayMicroseconds(500);
    
    // Send to game ESP
    esp_err_t result2 = esp_now_send(broadcastAddress2, (uint8_t *)&mySensors, sizeof(mySensors));
    if (result2 != ESP_OK) 
    {
      Serial.print("Game ESP send error: ");
      Serial.println(result2);
      callbacksExpected--;  // Won't get callback for failed send
    }
  }
  
  yield();
}