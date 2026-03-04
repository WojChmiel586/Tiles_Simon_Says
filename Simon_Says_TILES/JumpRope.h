#ifndef JumpRope_h
#define JumpRope_h

#include "Game.h"
#include "Colours.h"

class JumpRope : public Game {
public:
    JumpRope(Board& game_board);

    void Init() override;
    void Run(unsigned long dt) override;

    // Call this before Init()/Run() to set the difficulty level (1–5),
    void setLevel(int level);

private:

    enum jumpRopeState {
        GAMEPREP,
        ROW1TOP, ROW1MID, ROW1BOTT,
        ROW2TOP, ROW2MID, ROW2BOTT,
        ROW3TOP, ROW3MID, ROW3BOTT,
        ROW4TOP, ROW4MID, ROW4BOTT,
        ROW4BOTT_DONE,
        GAMEFEEDBACK,
        GAMEEND
    };

    jumpRopeState currentRopeState = GAMEPREP;

    // -------------------------------------------------------
    // Jump rope game variables
    // -------------------------------------------------------
    int  level       = 1;        // difficulty level 1–5
    int  jumpCount   = 0;
    int  jumpState   = 0;        // 0=none, 1=good, 2=partial, 3=bad
    int  liftOff     = 0;
    int  landing     = 0;
    int  startTile   = 0;
    int  endTile     = 4;

    uint32_t levColor = Colours::cyan;

    // Line delays (ms) per level
    static constexpr int lineDelays[5] = { 140, 180, 220, 260, 300 };
    int lineDelay  = lineDelays[0];

    static constexpr unsigned long interJump    = 3500; // wait between jumps
    static constexpr unsigned long resultDelay  = 300;  // feedback display time

    unsigned long startMillis    = 0;
    unsigned long feedbackMillis = 0;   // timer for non-blocking feedback display


    // Sensor helpers
    int getButtonInput() const { return board.getStructFront()[4].b; };

    // LED helpers
    // Light a row of 4 tiles (startTile..endTile-1) using a partial section
    void lightRow(Tile::LEDsections section, uint32_t color);

    // Feedback flashes
    void showFeedbackColor(uint32_t color);

    // -------------------------------------------------------
    // ESP-NOW sending helpers
    // -------------------------------------------------------
    void sendSoundMessage(int js, int jc);
    void sendResultMessage(int js, int jc);
};

#endif
