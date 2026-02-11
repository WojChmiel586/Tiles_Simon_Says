#include "esp_now.h"
#include "Board.h"
//static initialise
Board* Board::instance = nullptr;
volatile bool Board::newDataAvailable = false;
struct_message_all Board::boardsStructBack[7];


Board::Board() {
    // Reserve space for 16 pointers, initialize to nullptr
    tiles.reserve(TILE_COUNT);

    instance = this;

  memset(boardsStructBack, 0, sizeof(boardsStructBack));
  memset(boardsStructFront, 0, sizeof(boardsStructFront));
}


void Board::begin(int ledPins[]) 
{
    Serial.println("Board::begin() starting...");
    
    for (int i = 0; i < TILE_COUNT; i++) {
        Serial.print("Creating Tile ");
        Serial.println(i);
        
        tiles.emplace_back(std::make_unique<Tile>(ledPins[i]));
        tiles[i]->begin();
        tiles[i]->clear();
    }

    assignColours();
    
    Serial.println("Board::begin() complete");
}

void Board::assignColours() {
    for (int i = 0; i < TILE_COUNT; i++) 
    {
        uint16_t hue = (i * 65536UL) / TILE_COUNT;
        tiles[i]->setColour(tiles[i]->Strip().ColorHSV(hue, 255, 255));
    }
}

bool Board::initESPNow()
{
    //Initialize WiFi BEFORE ESP-NOW
    Serial.println("Initializing WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();  // Ensure clean state
    delay(100);

    //Initialize ESP-NOW
    Serial.println("Initializing ESP-NOW...");
    if (esp_now_init() != ESP_OK) 
    {
        Serial.println("Error initializing ESP-NOW");
        return false;
    }
    Serial.println("ESP-NOW initialized successfully");

    //Register callbacks
    esp_now_register_send_cb(Board::onDataSent);
    esp_now_register_recv_cb(esp_now_recv_cb_t(Board::onDataReceived));

    return true;
}

bool Board::addPeer(const uint8_t *macAddress)
{
    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    memcpy(peerInfo.peer_addr, macAddress, 6);

    if(esp_now_add_peer(&peerInfo) != ESP_OK)
    {
        Serial.println("Failed to add peer");
        return false;
    }
    Serial.println("Peer added successfully");
    return true;
}

void Board::setPeerAddresses(const uint8_t *buttons, const uint8_t *audio, const uint8_t *results)
{
    memcpy(buttonsAddress, buttons, 6);
    memcpy(audioAddress, audio, 6);
    memcpy(resultsAddress, results, 6);
}

bool Board::sendMessage(const uint8_t *macAddress, struct_message_all &message)
{
    esp_err_t result = esp_now_send(macAddress, (uint8_t *)&message, sizeof(message));
    if(result == ESP_OK)
    {
        Serial.println("Message sent successfully");
        return true;
    } 
    else 
    {
        Serial.print("Error sending message: ");
        Serial.println(result);
        return false;
    }
}

bool Board::sendToAudio(struct_message_all message)
{
    return sendMessage(audioAddress, message);
}

bool Board::sendToLaptop(struct_message_all message)
{
    return sendMessage(buttonsAddress, message);
}

bool Board::sendToResults(struct_message_all message)
{
    return sendMessage(resultsAddress, message);
}

void Board::onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void Board::onDataReceived(const uint8_t *mac_addr, const uint8_t *incomingData, int len)
{
    struct_message_all myData;
    memcpy(&myData, incomingData, sizeof(myData));

    int idx = myData.id - 1;

    if (idx < 0 || idx >= 7)
    {
        Serial.print("ERROR: Invalid board ID received: ");
        Serial.println(myData.id);
        return;
    }

    boardsStructBack[idx] = myData;
    newDataAvailable = true;
}

bool Board::hasNewData()
{
    return newDataAvailable;
}

void Board::processRecievedData()
{
    if  (newDataAvailable)
    {
        newDataAvailable = false;
        memcpy(boardsStructFront, boardsStructBack, sizeof(boardsStructBack));
        updateFromESPNOW(boardsStructFront);
    }
}

void Board::updateFromESPNOW(struct_message_all boards[]) {
    auto mapColumn = [&](int columnIndex, int idx0, int idx1, int idx2, int idx3) 
    {
        struct_message_all &s = boards[columnIndex];
        if (tiles[idx0]) tiles[idx0]->setSensors(s.dA, s.dB);
        if (tiles[idx1]) tiles[idx1]->setSensors(s.eA, s.eB);
        if (tiles[idx2]) tiles[idx2]->setSensors(s.fA, s.fB);
        if (tiles[idx3]) tiles[idx3]->setSensors(s.gA, s.gB);
    };

    mapColumn(0, 0, 1, 2, 3);   // ESP1
    mapColumn(1, 4, 5, 6, 7);   // ESP2
    mapColumn(2, 8, 9, 10, 11);  // ESP3
    mapColumn(3, 12, 13, 14, 15);  // ESP4
}

int Board::pressedTile() {
    for (int i = 0; i < TILE_COUNT; i++) {
        if (tiles[i] && tiles[i]->isPressed()) {
            return i;
        }
    }
    return -1;
}

void Board::lightAll()
{
    for (auto& tile : tiles)
    {
        tile->light();
    }
}

void Board::lightAll(uint32_t c)
{
    for (auto& tile : tiles)
    {
        tile->light(c);
    }
}

void Board::light(int i) {
    if (i < 0 || i >= TILE_COUNT || !tiles[i]) return;
    tiles[i]->light();
}

void Board::blinkBoard(uint32_t c)
{
    blinkFlag = !blinkFlag;
    if(blinkFlag)
    {
        lightAll(c);
    }
    else
    {
        clearAll();
    }
}

void Board::clear(int i) {
    if (i < 0 || i >= TILE_COUNT || !tiles[i]) return;
    tiles[i]->clear();
}

void Board::clearAll() {
    for (auto& tile : tiles) {
        if (tile) 
        {
            tile->clear();
        }
    }
}