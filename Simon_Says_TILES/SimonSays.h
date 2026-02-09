#ifndef SimonSays_h
#define SimonSays_h
#include "Game.h"
#include "Colours.h"

class SimonSays : public Game {
  public:

  SimonSays(Board& game_board);
  

  void Run(unsigned long dt) override;
  void Init() override;

  private:

  std::vector<int> game_sequence;
  std::vector<int> player_sequence;
  int sequenceIdx = 0;
  int prevSequenceLength = game_sequence.size();
  int lastTile = -1;
  unsigned long lastTileUpdate = 0;
  const unsigned long sequenceInterval = 800;
  unsigned long blinkTime = 0;
  const unsigned long blinkInterval = 200;
  unsigned long endWaitTime = 0;
  const unsigned long endWaitInterval = 1000;
  bool playerTurn = false;
  bool playerFailed = false;
  int finalScore = 0;


};
#endif