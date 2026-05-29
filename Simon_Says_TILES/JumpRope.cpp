#include "JumpRope.h"

// Static constexpr definitions
constexpr int JumpRope::lineDelays[5];

JumpRope::JumpRope(Board& game_board) : Game(game_board)
{
}

void JumpRope::setLevel(int lvl)
{
    level = lvl;

    if (level >= 1 && level <= 5) {
        lineDelay = lineDelays[level - 1];
    }

    switch (level) {
        case 1: levColor = Colours::lime;    break;
        case 2: levColor = Colours::cyan;    break;
        case 3: levColor = Colours::blue;    break;
        case 4: levColor = Colours::purple;  break;
        case 5: levColor = Colours::magenta; break;
        default: levColor = Colours::white;  break;
    }
}

void JumpRope::Init()
{
    setLevel(level);

    jumpCount        = 0;
    jumpState        = 0;
    liftOff          = 0;
    landing          = 0;
    startMillis      = 0;
    feedbackMillis   = 0;
    currentRopeState = GAMEPREP;

    board.clearAll();
}


void JumpRope::Run(unsigned long dt)
{
    unsigned long currentMillis = millis();

    //If button is pressed we change level and restart game - now handled via HandleInput()

    switch (currentRopeState) 
    {

        case GAMEPREP:
            liftOff          = 0;
            landing          = 0;
            startMillis      = currentMillis;
            currentRopeState = ROW1TOP;
            break;


        case ROW1TOP:
            if (currentMillis - startMillis >= interJump) {
                board.clearAll();
                startTile = 0; 
                endTile = 4;
                lightRow(Tile::TOP_LINE_HORIZONTAL, levColor);
                startMillis      = currentMillis;
                currentRopeState = ROW1MID;
            }
            break;

        case ROW1MID:
            if (currentMillis - startMillis >= lineDelay) {
                board.clearAll();
                startTile = 0; endTile = 4;
                lightRow(Tile::CENTRE_LINE_HORIZONTAL, levColor);
                startMillis      = currentMillis;
                currentRopeState = ROW1BOTT;
            }
            break;

        case ROW1BOTT:
            if (currentMillis - startMillis >= lineDelay) {
                board.clearAll();
                startTile = 0; endTile = 4;
                lightRow(Tile::BOTTOM_LINE_HORIZONTAL, levColor);
                startMillis      = currentMillis;
                currentRopeState = ROW2TOP;
            }
            break;

        case ROW2TOP:
            if (currentMillis - startMillis >= lineDelay) {
                startTile = 4; endTile = 8;
                lightRow(Tile::TOP_LINE_HORIZONTAL, levColor);
                startMillis      = currentMillis;
                currentRopeState = ROW2MID;
            }
            break;

        case ROW2MID:
            if (currentMillis - startMillis >= lineDelay) {
                board.clearAll();
                startTile = 4; endTile = 8;
                lightRow(Tile::CENTRE_LINE_HORIZONTAL, levColor);
                startMillis      = currentMillis;
                currentRopeState = ROW2BOTT;
            }
            break;

        case ROW2BOTT:
            if (currentMillis - startMillis >= lineDelay) {
                board.clearAll();
                startTile = 4; endTile = 8;
                lightRow(Tile::BOTTOM_LINE_HORIZONTAL, levColor);
                startMillis      = currentMillis;
                currentRopeState = ROW3TOP;
            }
            break;

        case ROW3TOP:
            if (currentMillis - startMillis >= lineDelay) {
                startTile = 8; endTile = 12;
                lightRow(Tile::TOP_LINE_HORIZONTAL, levColor);
                startMillis      = currentMillis;
                currentRopeState = ROW3MID;
            }
            break;

        case ROW3MID:
            if (currentMillis - startMillis >= lineDelay) {
                board.clearAll();
                startTile = 8; endTile = 12;
                lightRow(Tile::CENTRE_LINE_HORIZONTAL, levColor);
                startMillis      = currentMillis;
                currentRopeState = ROW3BOTT;
            }
            break;

        case ROW3BOTT:
            if (currentMillis - startMillis >= lineDelay) {
                board.clearAll();
                startTile = 8; endTile = 12;
                lightRow(Tile::BOTTOM_LINE_HORIZONTAL, levColor);
                startMillis      = currentMillis;
                currentRopeState = ROW4TOP;
            }
            break;

        case ROW4TOP:
            if (currentMillis - startMillis >= lineDelay) {
                startTile = 12; endTile = 16;
                lightRow(Tile::TOP_LINE_HORIZONTAL, levColor);
                startMillis      = currentMillis;
                currentRopeState = ROW4MID;
            }
            break;

        case ROW4MID:
            if (currentMillis - startMillis >= lineDelay) {
                board.clearAll();
                startTile = 12; endTile = 16;
                lightRow(Tile::CENTRE_LINE_HORIZONTAL, levColor);
                startMillis      = currentMillis;
                currentRopeState = ROW4BOTT;
            }
            break;

        // ----------------------------------------------------------------
        // ROW4BOTT – the rope has reached the floor; sample liftOff on T14
        // (tile index 13 in the 0-based board, matching sensorValue[26/27]
        //  in the original: boardsStruct[3].eA / eB = T14A / T14B).
        // ----------------------------------------------------------------
        case ROW4BOTT:
        {
            // Sample liftOff: if T14 (tile 13) has no weight → player jumped
            if (board.toeSensor(13) < Board::weightThreshold && board.heelSensor(13) < Board::weightThreshold) {
                liftOff = 0; // successfully jumped off
            } else {
                liftOff = 1; // too slow
            }

            if (currentMillis - startMillis >= lineDelay) {
                board.clearAll();
                startTile = 12; endTile = 16;
                lightRow(Tile::BOTTOM_LINE_HORIZONTAL, levColor);
                startMillis      = currentMillis;
                currentRopeState = ROW4BOTT_DONE;
            }
            break;
        }

        case ROW4BOTT_DONE:
        {
            if (board.toeSensor(14) < Board::weightThreshold && board.heelSensor(14) < Board::weightThreshold) {
                landing = 0; // still in the air
            } else {
                landing = 1; // landed too early
            }

            if (currentMillis - startMillis >= lineDelay) {
                board.clearAll();

                // jumpState: 3 = bad, 2 = partial, 1 = good  (matches original)
                jumpState = liftOff + landing + 1;
                jumpCount++;

                sendSoundMessage(jumpState, jumpCount);
                sendResultMessage(jumpState, jumpCount);


                startMillis      = currentMillis;
                currentRopeState = GAMEFEEDBACK;
            }
            break;
        }

        // ----------------------------------------------------------------
        // GAMEFEEDBACK – wait 800 ms (for sound), then flash result colour
        // for resultDelay ms, non-blocking.
        // ----------------------------------------------------------------
        case GAMEFEEDBACK:
        {
            // Phase 1: wait 800 ms before showing lights (sound plays first)
            if (feedbackMillis == 0 && currentMillis - startMillis >= 800) {
                uint32_t feedbackColor = Colours::white;
                if      (jumpState == 1) feedbackColor = Colours::green;
                else if (jumpState == 2) feedbackColor = Colours::yellow;
                else if (jumpState == 3) feedbackColor = Colours::red;

                board.lightAll(feedbackColor);
                feedbackMillis = currentMillis; // start the display timer
            }

            // Phase 2: after resultDelay ms, clear and advance
            if (feedbackMillis != 0 && currentMillis - feedbackMillis >= (unsigned long)(resultDelay * 1.5)) {
                board.clearAll();
                feedbackMillis = 0;

                if (jumpCount >= 6) {
                    // Set complete
                    jumpState = 0;
                    jumpCount = 0;
                    board.lightAll(Colours::white); // endJump equivalent
                    // We leave GAMEEND after another resultDelay so the white
                    // flash is visible; store time in startMillis for that.
                    startMillis      = currentMillis;
                    currentRopeState = GAMEEND;
                } else {
                    jumpState        = 0;
                    currentRopeState = GAMEPREP; // next jump
                }
            }
            break;
        }

        // ----------------------------------------------------------------
        // GAMEEND – idle, waiting for a level reset / new Init() call.
        // The white "endJump" light stays on until Init() is called again.
        // ----------------------------------------------------------------
        case GAMEEND:
            // Stay idle; caller can detect this state and trigger Init()
            break;
    }
}

// =============================================================================
// LED helpers
// =============================================================================

void JumpRope::lightRow(Tile::LEDsections section, uint32_t color)
{
    // startTile and endTile are set by the caller before each call, matching
    // the original global startTile/endTile pattern in rowtoplights() etc.
    for (int i = startTile; i < endTile; i++) {
        board.light(i, color, section);
    }
}

// =============================================================================
// ESP-NOW helpers
// =============================================================================

void JumpRope::sendSoundMessage(int js, int jc)
{
    struct_message_all msg;
    memset(&msg, 0, sizeof(msg));
    msg.id = 6;       // game ESP
    msg.jc = jc;
    msg.js = js;      // 0=none, 1=good, 2=partial, 3=bad
    board.sendToAudio(msg);
}

void JumpRope::sendResultMessage(int js, int jc)
{
    struct_message_all msg;
    memset(&msg, 0, sizeof(msg));
    msg.id = 6;
    msg.jc = jc;
    msg.js = js;
    msg.b = 1;
    // airtime is hard-coded to lineDelay for now; extend if needed
    msg.t  = lineDelay;
    board.sendToLaptop(msg);
}

// =============================================================================
// Input handling — called by the .ino when a button 1-5 is pressed
// =============================================================================

void JumpRope::HandleInput(int input)
{
    if (input >= 1 && input <= 5)
    {
        setLevel(input);
        Init();
        Serial.print("JumpRope: level set to ");
        Serial.println(input);
    }
}
