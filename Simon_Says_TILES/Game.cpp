#include "Game.h"

Game::Game(Board& game_board) : board(game_board)
{

}

void Game::Init()
{
  game_sequence.reserve(10);
  player_sequence.reserve(10);
  game_sequence.emplace_back(13);
}

void Game::Run()
{
      //SIMON SAYS LOGIC:
    //1. RANDOMISE A NUMBER BETWEEN 0 and 15 and add it to the array
    if (game_sequence.size() == prevSequenceLength) {
      int nextTile = random(16);
      game_sequence.emplace_back(nextTile);
    }
    //3. LIGHT UP THE FIRST TILE IN A SEQUENCE AND WAIT FOR A SET TIME
    //4. TURN OFF THE PREVIOUS LIGHT (OPTIONAL DEPENDING ON DIFFICULTY)
    //5. TURN ON THE NEXT LIGHT IN THE SEQUENCE
    //6. REPEAT 3-5 UNTIL NO MORE NUMBERS IN THE SEQUENCE
    if (millis() - lastTileUpdate >= sequenceInterval && !playerTurn) {
      //GO THROUGH EACH TILE NUMBER IN SEQUENCE
      if (sequenceIdx < game_sequence.size()) {
        board.light(game_sequence.at(sequenceIdx));
        lastTileUpdate = millis();
        sequenceIdx++;
      }
      //WE FINISHED THE SEQUENCE
      else {
        //CLEAR THE BOARD LIGHTS
        board.clearAll();
        sequenceIdx = 0;
        playerTurn = true;
      }
    }
    //PLAYERS TURN LOGIC:
    //8. WAIT FOR PLAYER TO STEP ON TILE
    //9. IF CORRECT TILE STEPPED ON, WAIT FOR NEXT INPUT OTHERWISE END THE GAME
    //10 ONCE THE SEQUENCE IS COMPLETE, INDICATE A TILE FOR PLAYER TO STAND ON TO CONTINUE GAME
    //11 REPEAT UNTIL FAILURE
    if (playerTurn) {
      int pressedTile = board.pressedTile();

      //CHECK IF ANY VALID TILE IS PRESSED
      if (pressedTile != -1 && pressedTile < 16) {
        //CHECK IF TILE PRESSED CORRESPONDS TO THE TILE IN GAME SEQUENCE
        if (pressedTile == game_sequence.at(sequenceIdx) && pressedTile != lastTile) {
          //CODE WHEN TILE MATCHES SEQUENCE
          board.light(pressedTile);
          sequenceIdx++;
          lastTile = pressedTile;
          if(sequenceIdx >= game_sequence.size())
          {
            //PLAYER FINISHED THE SEQUENCE SUCCESFULLY
            prevSequenceLength = game_sequence.size();
            board.clearAll();
            playerTurn = false;
            sequenceIdx = 0;
          }
        }
        else {
          //CODE WHEN TILE DOESN'T MATCH SEQUENCE
          //SOME SORT OF INDICATION OF FAILURE

          //CLEAR BOARD AND RESTART
          board.clearAll();
          game_sequence.clear();
          game_sequence.emplace_back(13);
          sequenceIdx = 0;
          prevSequenceLength = 0;
          playerFailed = true;
        }
      }
    }

    //END STATE FOR THE GAME WHEN PLAYER FAILS
    if(playerFailed)
    {
      //BUTTON CONTROL OR SOMETHING TO RESTART THE GAME?
    }

}