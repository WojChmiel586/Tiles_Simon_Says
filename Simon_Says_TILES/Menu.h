#ifndef Menu_h
#define Menu_h

#include "Game.h"
#include "Colours.h"


class Menu : public Game 
{
  public:
  Menu(Board& game_board);

  void Init() override;
  void Run(unsigned long dt) override;

  private:

  bool sectionTest = false;
  unsigned long testInterval = 1000;
  unsigned long testTimer = 0;
  int section = 0;

};
#endif