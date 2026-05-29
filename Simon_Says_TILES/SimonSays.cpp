#include "SimonSays.h"

SimonSays::SimonSays(Board& game_board) : Game(game_board)
{

}

void SimonSays::HandleInput(int input)
{
  // Buttons 1-5 available for future SimonSays in-game options
  // e.g. difficulty, hints, etc.
  (void)input;
}

void SimonSays::Init()
{
  game_sequence.clear();
  player_sequence.clear();
  board.clearAll();
  playerTurn = false;
  playerFailed = false;
  blinkCount = 0;
  sequenceIdx = 0;
  lastTileUpdate = 0;
  lastTile = -1;
  prevSequenceLength = 0;
  game_sequence.reserve(10);
  player_sequence.reserve(10);
  game_sequence.emplace_back(13);
}

void SimonSays::Run(unsigned long dt)
{

    if(millis() - endWaitTime <= endWaitInterval && game_sequence.size() > 1)
    {
      return;
    }
    //SIMON SAYS LOGIC:
    //1. RANDOMISE A NUMBER BETWEEN 0 and 15 and add it to the array
    if (game_sequence.size() == prevSequenceLength) 
    {
      board.clearAll();
      //Add all possible tiles to a temporarly list <---- THIS CAN BE EXPANDED FOR HIGHER DIFFICULTIES
      std::vector<int> possibleTiles;
      int previous = game_sequence.back();
      if(previous + 1 <= 15 && previous + 1 >= 0 && previous % 4 != 3)
      {
        possibleTiles.emplace_back(previous + 1);
      }
      if(previous - 1 <= 15 && previous - 1 >= 0 && previous % 4 != 0)
      {
        possibleTiles.emplace_back(previous - 1);
      }
      if(game_sequence.back() + 4 <= 15 && game_sequence.back() + 4 >= 0)
      {
        possibleTiles.emplace_back(previous + 4);
      }
      if(game_sequence.back() - 4 <= 15 && game_sequence.back() - 4 >= 0)
      {
        possibleTiles.emplace_back(previous - 4);
      }

      //randomly choose one of these possible tiles and assign it to the sequence
      int nextTile = possibleTiles[random(0,possibleTiles.size())];
      game_sequence.emplace_back(nextTile);

      for (auto tile : game_sequence)
      {
        Serial.print("Tile in the sequence: " );
        Serial.println(tile);
      }
    }

    //3. LIGHT UP THE FIRST TILE IN A SEQUENCE AND WAIT FOR A SET TIME
    //4. TURN OFF THE PREVIOUS LIGHT (OPTIONAL DEPENDING ON DIFFICULTY)
    //5. TURN ON THE NEXT LIGHT IN THE SEQUENCE
    //6. REPEAT 3-5 UNTIL NO MORE NUMBERS IN THE SEQUENCE
    if (millis() - lastTileUpdate >= sequenceInterval && !playerTurn)
    {
      //GO THROUGH EACH TILE NUMBER IN SEQUENCE
      if (sequenceIdx < game_sequence.size()) {
        board.light(game_sequence.at(sequenceIdx));

        if (sequenceIdx > 0)
        {
          board.clear(game_sequence.at(sequenceIdx-1));
        }
        lastTileUpdate = millis();
        sequenceIdx++;
      }
      //WE FINISHED THE SEQUENCE
      else
      {
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
    if (playerTurn && !playerFailed) {
      int pressedTile = board.pressedTile();


      //CHECK IF ANY VALID TILE IS PRESSED
      if (pressedTile >= 0 && pressedTile < 16) {
        //CHECK IF TILE PRESSED CORRESPONDS TO THE TILE IN GAME SEQUENCE

        //CODE WHEN TILE MATCHES SEQUENCE
        if (pressedTile == game_sequence.at(sequenceIdx) && pressedTile != lastTile) {
          //Light up stepped on tile
          board.light(pressedTile);
          //Turn off the previous tile (if any exist)
          if(lastTile >= 0 && lastTile <= 15)
          {
            board.clear(lastTile);
          }
          sequenceIdx++;
          lastTile = pressedTile;

          if(sequenceIdx >= game_sequence.size())
          {
            //PLAYER FINISHED THE SEQUENCE SUCCESFULLY
            prevSequenceLength = game_sequence.size();

            //Send Succcess sounds to audio ESP
            struct_message_all audioMessage;
            audioMessage.id = 6;
            audioMessage.js = 1;
            board.sendToAudio(audioMessage);

            playerTurn = false;
            sequenceIdx = 0;
            lastTileUpdate = 0;
            lastTile = -1;
            endWaitTime = millis();
          }
        }
        else if (pressedTile != game_sequence.at(sequenceIdx) && pressedTile != lastTile && sequenceIdx != 0) {
          //CODE WHEN TILE DOESN'T MATCH SEQUENCE
          //SOME SORT OF INDICATION OF FAILURE
          //Send Succcess sounds to audio ESP
          struct_message_all audioMessage;
          audioMessage.id = 6;
          audioMessage.js = 3;
          board.sendToAudio(audioMessage);
          finalScore = game_sequence.size();
          playerFailed = true;
          blinkTime = millis();  // start blink timer immediately
        }
      }
    }
    //END STATE FOR THE GAME WHEN PLAYER FAILS
    if(playerFailed)
    {
      if (millis() - blinkTime >= blinkInterval)
      {
        blinkTime = millis();
        board.blinkBoard(Colours::red);
        blinkCount++;

        if (blinkCount >= FAIL_BLINKS * 2)  // *2 because each blink = on + off
        {
          struct_message_all simonMessage;
          simonMessage.id = 6;
          simonMessage.eA = finalScore - 1;
          board.sendToResults(simonMessage);
          Init();  // restart from the beginning
        }
      }
    }

}