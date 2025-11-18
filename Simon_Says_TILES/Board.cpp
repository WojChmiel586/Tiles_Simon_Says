#include "Board.h"

Board::Board() {}

void Board::begin() 
{
    for (int i = 0; i < TILE_COUNT; i++)
        tiles[i].begin();
}

void Board::assignColours() {
    for (int i = 0; i < TILE_COUNT; i++) 
    {
        uint16_t hue = (i * 65536UL) / TILE_COUNT;
        tiles[i].setColour(tiles[i].Strip().ColorHSV(hue, 255, 255));
    }
}

int Board::pressedTile() 
{
    for (int i = 0; i < TILE_COUNT; i++)
        if (tiles[i].isPressed())
            return i;
    return -1;
}

void Board::light(int i) 
{
    tiles[i].light();
}

void Board::clear(int i) 
{
    tiles[i].clear();
}

void Board::clearAll() 
{
    for (int i = 0; i < TILE_COUNT; i++)
        tiles[i].clear();
}