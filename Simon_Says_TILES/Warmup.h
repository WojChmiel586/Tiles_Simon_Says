#ifndef Warmup_h
#define Warmup_h
#include "Game.h"

class Warmup : public Game
{
  public:
  Warmup(Board& game_board);
  
  void Run(unsigned long dt) override;
  void Init() override;


  private:


};
#endif