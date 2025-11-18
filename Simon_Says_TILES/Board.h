#pragma once
#include "Tile.h"

class Board {
public:
    static const int TILE_COUNT = 16;
    Tile tiles[TILE_COUNT];

    Board();

    void begin();
    void assignColours();

    int pressedTile(); // first pressed tile index or -1
    void light(int i);
    void clear(int i);
    void clearAll();
};