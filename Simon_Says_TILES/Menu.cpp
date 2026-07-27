#include "esp32-hal.h"
#include "Menu.h"


Menu::Menu(Board& game_board) : Game(game_board)
{

}

void Menu::Init()
{
  board.clearAll();
  board.wipeResults();
  board.PlaySong(Audio::SONGS::SONG_1);
}

void Menu::Run(unsigned long dt)
{
  if(millis() - testTimer >= testInterval)
  {
    sectionTest = false;
    testTimer = millis();
    section++;
    board.clearAll();
    if(section >= 12)
    section = 0;
  }
  if (!sectionTest)
  {
    Tile::LEDsections m_section = static_cast<Tile::LEDsections>(section);
    board.light(0, Colours::blue, m_section);
    board.light(1, Colours::blue, m_section);
    board.light(2, Colours::blue, m_section);
    board.light(3, Colours::blue, m_section);
    board.light(4, Colours::red, m_section);
    board.light(5, Colours::red, m_section);
    board.light(6, Colours::red, m_section);
    board.light(7, Colours::red, m_section);
    board.light(8, Colours::green, m_section);
    board.light(9, Colours::green, m_section);
    board.light(10, Colours::green, m_section);
    board.light(11, Colours::green, m_section);
    board.light(12, Colours::white, m_section);
    board.light(13, Colours::white, m_section);
    board.light(14, Colours::white, m_section);
    board.light(15, Colours::white, m_section);

    sectionTest = true;
  }
  
}

void Menu::HandleInput(int input)
{
  // Buttons 1-5 available for future Warmup in-game options
  (void)input;
}