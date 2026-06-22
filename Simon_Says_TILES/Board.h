#include <stdint.h>
#pragma once
#include <esp_now.h>
#include <WiFi.h>
#include <vector>
#include <memory>
#include "Colours.h"
#include "Tile.h"
#include "ESPNowStruct.h"

class Board {
public:
    static const int TILE_COUNT = 16;
    static const int weightThreshold = 500;

    Board();

    //Initialisation functions
    void begin(int ledPins[]);
    void assignColours();

    //ESP-NOW setup
    bool initESPNow();
    bool addPeer(const uint8_t* macAddress);
    void setPeerAddresses(const uint8_t* buttons, const uint8_t* audio, const uint8_t* results);

    //ESP-NOW sending
    bool sendMessage(const uint8_t* macAddress, struct_message_all& message);
    bool sendToAudio(struct_message_all message);
    bool sendToAudio(int id, int trackNumber = 0, int SfxNumber = 0);
    bool sendToLaptop(struct_message_all message);
    bool sendToResults(struct_message_all message);

    //Static callbacks for ESP-NOW
    static void onDataSent(const esp_now_send_info_t* send_info, esp_now_send_status_t status);
    static void onDataReceived(const uint8_t *mac_addr, const uint8_t *incomingData, int len);

    //Recieve data
    bool hasNewData();
    void processRecievedData();
    void updateFromESPNOW(struct_message_all boards[]);
    struct_message_all* getStructFront() {return boardsStructFront; };
    void wipeResults();


    //Tile interaction
    int                 pressedTile(); // first pressed tile index or -1
    bool                tilePressed(int idx, int weightOn = weightThreshold) const;
    std::vector<int>    tilesPressed();
    int                 toeSensor(int idx) const;
    int                 heelSensor(int idx) const;
    bool                toePressed(int idx, int weightOn = weightThreshold);
    bool                heelPressed(int idx, int weightOn = weightThreshold);
    void                lightAll();
    void                lightAll(uint32_t c);
    void                lightAllSection(uint32_t c, int section);
    void                light(int i, uint32_t c = 0, Tile::LEDsections section = Tile::LEDsections::WHOLE);
    void                blinkBoard(uint32_t c);
    void                clear(int i);
    void                clearAll();
    std::vector<std::unique_ptr<Tile>> tiles;


private:

//Static board pointer
static Board* instance;

//ESP-NOW data handling
static volatile bool newDataAvailable;
static struct_message_all boardsStructBack[7];
struct_message_all boardsStructFront[7];
struct_message_all myData;

//MAC addresses
uint8_t buttonsAddress[6];
uint8_t audioAddress[6];
uint8_t resultsAddress[6];

//Misc
bool blinkFlag = false;



};