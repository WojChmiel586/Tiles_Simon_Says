#pragma once
#include "Tile.h"
#include "ESPNowStruct.h"
#include <vector>
#include <memory>

class Board {
public:
    static const int TILE_COUNT = 16;


    Board();

    void begin(int ledPins[]);
    void assignColours();

    void updateFromESPNOW(struct_message_all boards[]);

    int pressedTile(); // first pressed tile index or -1
    void lightAll();
    void lightAll(uint32_t c);
    void light(int i);
    void blinkBoard(uint32_t c);
    void clear(int i);
    void clearAll();
    std::vector<std::unique_ptr<Tile>> tiles;

private:
bool blinkFlag = false;
};