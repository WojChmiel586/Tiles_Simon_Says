#ifndef Game_h
#define Game_h
#include "Board.h"
#include <vector>
#include <memory>
#include "AudioEnums.h"


class Game
{
  public:
    Game(Board& game_board);
    virtual void Run(unsigned long dt) = 0;
    virtual void Init() = 0;
    virtual void HandleInput(int input) = 0;  // in-game button handling (values 1-5)

  protected:
    Board& board;
    unsigned long deltaTime = 0;

  private:
};
#endif