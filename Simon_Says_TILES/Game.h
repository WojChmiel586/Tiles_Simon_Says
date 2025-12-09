#ifndef Game_h
#define Game_h
#include "Board.h"
#include <vector>
#include <memory>

class Game
{
  public:
    Game(Board& game_board);
    void Run();
    void Init();

  private:
    Board& board;
    //Game variables
    std::vector<int> game_sequence;
    std::vector<int> player_sequence;
    int sequenceIdx = 0;
    int prevSequenceLength = game_sequence.size();
    int lastTile = -1;
    unsigned long lastTileUpdate = 0;
    const unsigned long sequenceInterval = 400;
    bool playerTurn = false;
    bool playerFailed = false;
};
#endif