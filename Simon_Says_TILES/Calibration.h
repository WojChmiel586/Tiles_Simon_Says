#ifndef Calibration_h
#define Calibration_h

#include "Game.h"
#include "Colours.h"

// =============================================================================
//  Calibration
//  A single Game subclass that houses all 7 calibration exercises plus the
//  end-of-session display (calibend).  Call setExercise(n) to select which
//  exercise to run, then Init() to reset it, then Run() every loop tick.
//
//  Exercise mapping (set via setExercise(), called from .ino HandleInput routing):
//    91 → Calib 1 – Marching on the spot
//    92 → Calib 2 – Static balance (L / R / Toes / Heels)
//    93 → Calib 3 – Side leg lifts
//    94 → Calib 4 – Clockface agility taps (2 reps of 16 taps)
//    91 → Calib 5 – Alternating lunges      (values TBD, will be reassigned)
//    92 → Calib 6 – Narrow + wide squats    (values TBD, will be reassigned)
//    93 → Calib 7 – Airtime jumps × 3       (values TBD, will be reassigned)
//    94 → Calibend – End-of-session display  (values TBD, will be reassigned)
//  NOTE: Button values 95-98 are reserved for game selection in the .ino.
// =============================================================================

class Calibration : public Game {
public:
    Calibration(Board& game_board);

    // Select which exercise to run (91–98).  Call before Init().
    void setExercise(int exerciseNumber);

    void Init() override;
    void Run(unsigned long dt) override;
    void HandleInput(int input) override;  // not used internally; stub to satisfy Game interface

    // Returns true once the current exercise has completed (is in its idle state).
    bool isDone() const;

private:
    // =========================================================================
    // Exercise selector
    // =========================================================================
    int exercise = 91;

    // =========================================================================
    // Shared timing / scoring variables (mirror the originals)
    // =========================================================================
    static constexpr int weightOn       = 400;
    static constexpr int stepDelay      = 500;
    static constexpr int stepDelaymob   = (int)(stepDelay * 1.5); // 750
    static constexpr int balanceDelay   = stepDelay * 20;          // 10 000 ms
    static constexpr int sideliftDelay  = stepDelay * 4;           // 2 000 ms
    static constexpr int lungeDelay     = stepDelay * 4;           // 2 000 ms
    static constexpr int squat1Delay    = stepDelay * 4;           // 2 000 ms
    static constexpr int squat2Delay    = stepDelay * 8;           // 4 000 ms
    static constexpr int airtimeDefault = 550;

    unsigned long calib2Results[4] = {0,0,0,0};
    int exCounter        = 0;
    int balanceScore     = 0;
    int balChecker       = 0;
    int balanceAchieved  = 0;
    int balanceScoreSide = 0;
    int balanceLeft      = 0;
    int balanceRight     = 0;
    int balanceScoreDyn  = 0;
    int maxcount         = 0;
    int mobScore         = 0;
    int tap              = 0;
    int tapcounter       = 0;
    int strengthScore    = 0;
    int squatCounter     = 0;
    int squatScore       = 0;

    // Sensor array (32 values), loaded from board tiles each Run() tick
    int sensorValue[32] = {};

    unsigned long startMillis = 0;
    int flashIdx = 0;  // used by runCalibEnd for non-blocking sequential tile flash

    // =========================================================================
    // Per-exercise state machines
    // =========================================================================

    // --- Calib 1: Marching ---
    enum MarchState { MARCHPREP, MARCHSTART, MARCH1END, MARCH2END, MARCHDONE };
    MarchState marchState = MARCHPREP;

    // --- Calib 2: Static balance ---
    enum BalState {
        BAL1PREP, BAL1START, BAL1END,
        BAL2START, BAL2END,
        BAL3START, BAL3END,
        BAL4START, BAL4END,
        BALDONE
    };
    BalState balState = BAL1PREP;

    // --- Calib 3: Side leg lifts ---
    enum SideState { SIDEPREP, SIDE1START, SIDE1END, SIDE2START, SIDE2END, SIDEDONE };
    SideState sideState = SIDEPREP;

    // --- Calib 4: Clockface agility ---
    enum CalibState {
        STEP1_PREP,
        STEP1_WAIT_PREP_OFF_LIGHTS_ON,
        STEP1_LIGHTS_ON_WAIT_TAP,  STEP1_STEPBACK_LIGHTS2_ON,
        STEP2_LIGHTS_ON_WAIT_TAP,  STEP2_STEPBACK_LIGHTS3_ON,
        STEP3_LIGHTS_ON_WAIT_TAP,  STEP3_STEPBACK_LIGHTS4_ON,
        STEP4_LIGHTS_ON_WAIT_TAP,  STEP4_STEPBACK_LIGHTS5_ON,
        STEP5_LIGHTS_ON_WAIT_TAP,  STEP5_STEPBACK_LIGHTS6_ON,
        STEP6_LIGHTS_ON_WAIT_TAP,  STEP6_STEPBACK_LIGHTS7_ON,
        STEP7_LIGHTS_ON_WAIT_TAP,  STEP7_STEPBACK_LIGHTS8_ON,
        STEP8_LIGHTS_ON_WAIT_TAP,
        STEP9_LIGHTS_ON_WAIT_TAP,  STEP9_STEPBACK_LIGHTS10_ON,
        STEP10_LIGHTS_ON_WAIT_TAP, STEP10_STEPBACK_LIGHTS11_ON,
        STEP11_LIGHTS_ON_WAIT_TAP, STEP11_STEPBACK_LIGHTS12_ON,
        STEP12_LIGHTS_ON_WAIT_TAP, STEP12_STEPBACK_LIGHTS13_ON,
        STEP13_LIGHTS_ON_WAIT_TAP, STEP13_STEPBACK_LIGHTS14_ON,
        STEP14_LIGHTS_ON_WAIT_TAP, STEP14_STEPBACK_LIGHTS15_ON,
        STEP15_LIGHTS_ON_WAIT_TAP, STEP15_STEPBACK_LIGHTS16_ON,
        STEP16_LIGHTS_ON_WAIT_TAP,
        STEP16_DONE
    };
    CalibState calibState = STEP1_PREP;

    // --- Calib 5: Lunges ---
    enum LungeState { LUNGEPREP, LUNGE1START, LUNGE1END, LUNGE2START, LUNGE2END, LUNGEDONE };
    LungeState lungeState = LUNGEPREP;

    // --- Calib 6: Squats ---
    enum SquatState {
        SQUATPREP,
        SQUAT1START, SQUAT1END,
        SQUAT2START, SQUAT2END,
        SQUAT3START, SQUAT3END,
        SQUAT4PREP,
        SQUAT4START, SQUAT4END,
        SQUAT5START, SQUAT5END,
        SQUAT6START, SQUAT6END,
        SQUATDONE
    };
    SquatState squatState = SQUATPREP;

    // --- Calib 7: Jumps ---
    enum JumpState { JUMPPREP, JUMPBENCHMARK, JUMPBALANCE ,JUMPSTART, JUMPOFF, JUMPLANDPREP, JUMPLAND, JUMPEND, JUMPDONE };
    JumpState jumpState = JUMPPREP;

    // --- Calibend ---
    enum FinalState { FINAL, FINAL_FLASH, FINAL_HOLD, FINALDONE };
    FinalState finalState = FINAL;

    // =========================================================================
    // Private helpers
    // =========================================================================

    // Load sensor data from board tiles into sensorValue[]
    void loadSensorData();

    // Sensor accessors
    int  toe(int tileIdx)  const;
    int  heel(int tileIdx) const;
    bool pressed(int tileIdx) const;

    // LED helpers (translate original strip-direct calls to board.light())
    void prepCalib4();                       // T10 RIGHT_HALF cyan + T11 LEFT_HALF magenta
    void stepBackLeft();                     // T10 RIGHT_HALF cyan
    void stepBackRight();                    // T11 LEFT_HALF magenta
    void stepBackLeftTap();
    void stepBackRightTap();
    void clearT14();
    void clearT15();
    void clearT14T15();
    void clearRow3();                        // tiles 8-11
    void clearRow2();                        // tiles 4-7
    void clearLungeTiles();                  // tiles 5,6,9,10,13,14

    // Tap-tile lights: tileIdx = 0-based board tile, section and colour
    void lightTapTile(int tileIdx, Tile::LEDsections section, uint32_t color);
    void clearTile(int tileIdx);

    // Shared scoring/tap helper used by every calib4 tap step
    void processTap(int sensorIdx);          // checks sensor, sets tapcounter
    void processTapAvg(int sA, int sB);      // average of two sensors
    void finishTapStep(int tileIdx, bool stepbackIsLeft, CalibState nextState);

    // ESP-NOW result sender
    void sendResult(struct_message_all msg);

    // Per-exercise run functions
    void runCalib1(unsigned long now);
    void runCalib2(unsigned long now);
    void runCalib3(unsigned long now);
    void runCalib4(unsigned long now);
    void runCalib5(unsigned long now);
    void runCalib6(unsigned long now);
    void runCalib7(unsigned long now);
    void runCalibEnd(unsigned long now);
};

#endif
