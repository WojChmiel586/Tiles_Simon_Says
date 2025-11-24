#include "Board.h"

Board::Board() {}

void Board::begin(int ledPins[]) 
{
    for (int i = 0; i < TILE_COUNT; i++)
    {
        tiles[i] = Tile(ledPins[i]);
        tiles[i].begin();
        tiles[i].clear();
    }
}

void Board::assignColours() {
    for (int i = 0; i < TILE_COUNT; i++) 
    {
        uint16_t hue = (i * 65536UL) / TILE_COUNT;
        tiles[i].setColour(tiles[i].Strip().ColorHSV(hue, 255, 255));
    }
}

void Board::updateFromESPNOW(struct_message_all boards[])
{
    // boards[0] = ESP1 => tiles 1,5,9,13  -> indices 0,4,8,12
    // boards[1] = ESP2 => tiles 2,6,10,14 -> indices 1,5,9,13
    // boards[2] = ESP3 => tiles 3,7,11,15 -> indices 2,6,10,14
    // boards[3] = ESP4 => tiles 4,8,12,16 -> indices 3,7,11,15
    // boards[4] = ESP6 => game ESP metadata (airtime etc)
    // boards[5] = ESP7 => weights 

    auto mapColumn = [&](int columnIndex, int idx0, int idx1, int idx2, int idx3)
    {
        struct_message_all &s = boards[columnIndex];
        tiles[idx0].setSensors(s.dA, s.dB);
        tiles[idx1].setSensors(s.eA, s.eB);
        tiles[idx2].setSensors(s.fA, s.fB);
        tiles[idx3].setSensors(s.gA, s.gB);
    };

    mapColumn(0,0,4,8,12); //ESP1
    mapColumn(1,1,5,9,13); //ESP2
    mapColumn(2,2,6,10,14);//ESP3
    mapColumn(3,3,7,11,15);//ESP4

}

int Board::pressedTile() 
{
    for (int i = 0; i < TILE_COUNT; i++)
    {
        if (tiles[i].isPressed())
        {
            return i;
        }
    }
    return -1;
}

void Board::light(int i) 
{
    if (i<0 || i >= TILE_COUNT) return;
    tiles[i].light();
}

void Board::clear(int i) 
{
    if (i<0 || i >= TILE_COUNT) return;
    tiles[i].clear();
}

void Board::clearAll() 
{
    for (int i = 0; i < TILE_COUNT; i++)
        tiles[i].clear();
}