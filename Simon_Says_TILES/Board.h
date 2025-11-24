#pragma once
#include "Tile.h"
#include "ESPNowStruct.h"

class Board {
public:
    static const int TILE_COUNT = 16;


    Board();

    void begin(int ledPins[]);
    void assignColours();

    void updateFromESPNOW(struct_message_all boards[]);

    int pressedTile(); // first pressed tile index or -1
    void light(int i);
    void clear(int i);
    void clearAll();

    Tile tiles[TILE_COUNT];

private:
};