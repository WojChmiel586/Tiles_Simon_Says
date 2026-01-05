#include "Board.h"

Board::Board() {
    // Reserve space for 16 pointers, initialize to nullptr
    tiles.reserve(TILE_COUNT);
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
    
    Serial.println("Board::begin() complete");
}

void Board::assignColours() {
    for (int i = 0; i < TILE_COUNT; i++) 
    {
        uint16_t hue = (i * 65536UL) / TILE_COUNT;
        tiles[i]->setColour(tiles[i]->Strip().ColorHSV(hue, 255, 255));
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