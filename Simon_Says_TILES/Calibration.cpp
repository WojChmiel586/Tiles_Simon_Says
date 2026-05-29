#include "Calibration.h"

// =============================================================================
// Constructor / setExercise / Init / isDone
// =============================================================================

Calibration::Calibration(Board& game_board) : Game(game_board) {}

void Calibration::setExercise(int exerciseNumber)
{
    exercise = exerciseNumber;
    Init();
}

void Calibration::Init()
{
    board.clearAll();
    loadSensorData();
    startMillis      = millis();
    exCounter        = 0;
    balanceScore     = 0;
    balChecker       = 0;
    balanceAchieved  = 0;
    balanceScoreSide = 0;
    balanceScoreDyn  = 0;
    maxcount         = 0;
    mobScore         = 0;
    tap              = 0;
    tapcounter       = 0;
    strengthScore    = 0;
    squatCounter     = 0;
    squatScore       = 0;

    // Reset every sub-state machine so re-triggering always starts fresh
    marchState = MARCHPREP;
    balState   = BAL1PREP;
    sideState  = SIDEPREP;
    calibState = STEP1_PREP;
    lungeState = LUNGEPREP;
    squatState = SQUATPREP;
    jumpState  = JUMPPREP;
    finalState = FINAL;

    board.clearAll();
}

void Calibration::HandleInput(int input)
{
    setExercise(input);
}

bool Calibration::isDone() const
{
    switch (exercise) {
        case 91: return marchState == MARCHDONE;
        case 92: return balState   == BALDONE;
        case 93: return sideState  == SIDEDONE;
        case 94: return calibState == STEP16_DONE;
        case 95: return lungeState == LUNGEDONE;
        case 96: return squatState == SQUATDONE;
        case 97: return jumpState  == JUMPDONE;
        case 98: return finalState == FINALDONE;
        default: return true;
    }
}

// =============================================================================
// Run – dispatcher
// =============================================================================

void Calibration::Run(unsigned long /*dt*/)
{
    loadSensorData();
    unsigned long now = millis();

    switch (exercise) {
        case 91: runCalib1(now);   break;
        case 92: runCalib2(now);   break;
        case 93: runCalib3(now);   break;
        case 94: runCalib4(now);   break;
        case 95: runCalib5(now);   break;
        case 96: runCalib6(now);   break;
        case 97: runCalib7(now);   break;
        case 98: runCalibEnd(now); break;
        default: break;
    }
}

// =============================================================================
// Sensor helpers
// =============================================================================

void Calibration::loadSensorData()
{
    // Mirrors the original loadSensorData() layout:
    //   board tiles 0-3   = ESP1 (T1-T4),  sensorValue[0-7]
    //   board tiles 4-7   = ESP2 (T5-T8),  sensorValue[8-15]
    //   board tiles 8-11  = ESP3 (T9-T12), sensorValue[16-23]
    //   board tiles 12-15 = ESP4 (T13-T16),sensorValue[24-31]
    for (int t = 0; t < 16; t++) {
        sensorValue[t * 2]     = board.tiles[t]->getToeSensor();
        sensorValue[t * 2 + 1] = board.tiles[t]->getHeelSensor();
    }
}

int Calibration::toe(int idx)  const { return (idx >= 0 && idx < 16) ? board.tiles[idx]->getToeSensor()  : 0; }
int Calibration::heel(int idx) const { return (idx >= 0 && idx < 16) ? board.tiles[idx]->getHeelSensor() : 0; }
bool Calibration::pressed(int idx) const { return toe(idx) > weightOn || heel(idx) > weightOn; }

// =============================================================================
// LED helpers
// =============================================================================

// T10 = board tile index 9  (row 3, col 2, 0-based)
// T11 = board tile index 10 (row 3, col 3, 0-based)
// The original strip10/strip11 are 0-based array positions 9 and 10.

void Calibration::prepCalib4()
{
    board.light(9,  Colours::cyan,    Tile::RIGHT_HALF);  // T10: right half, left colour
    board.light(10, Colours::magenta, Tile::LEFT_HALF);   // T11: left half, right colour
}

void Calibration::stepBackLeft()
{
    board.light(9, Colours::cyan, Tile::RIGHT_HALF);
}

void Calibration::stepBackRight()
{
    board.light(10, Colours::magenta, Tile::LEFT_HALF);
}

void Calibration::clearT10()  { board.clear(9);  }
void Calibration::clearT11()  { board.clear(10); }

void Calibration::clearT10T11()
{
    board.clear(9);
    board.clear(10);
}

void Calibration::clearRow3()
{
    // Tiles 8-11 (T9-T12)
    for (int i = 8; i <= 11; i++) board.clear(i);
}

void Calibration::clearRow2()
{
    // Tiles 4-7 (T5-T8)
    for (int i = 4; i <= 7; i++) board.clear(i);
}

void Calibration::clearLungeTiles()
{
    // T6=5, T7=6, T10=9, T11=10, T14=13, T15=14
    for (int i : {5, 6, 9, 10, 13, 14}) board.clear(i);
}

void Calibration::lightTapTile(int tileIdx, Tile::LEDsections section, uint32_t color)
{
    board.light(tileIdx, color, section);
}

void Calibration::clearTile(int tileIdx)
{
    board.clear(tileIdx);
}

// =============================================================================
// Tap helpers used by calib4
// =============================================================================

void Calibration::processTap(int sensorIdx)
{
    tap = 0;
    if (sensorValue[sensorIdx] > weightOn) {
        tap++;
        tapcounter = 100;
    }
}

void Calibration::processTapAvg(int sA, int sB)
{
    tap = 0;
    int avg = (sensorValue[sA] + sensorValue[sB]) / 2;
    if (avg > weightOn) {
        tap++;
        tapcounter = 100;
    }
}

// Called at the end of a tap window: accumulates score, clears tile,
// turns on the step-back light, resets timer.
void Calibration::finishTapStep(int tileIdx, bool stepbackIsLeft, CalibState nextState)
{
    clearTile(tileIdx);
    maxcount += tapcounter;
    Serial.print(", total score: ");
    Serial.println(maxcount);
    mobScore++;
    tapcounter = 0;
    if (stepbackIsLeft) stepBackLeft(); else stepBackRight();
    startMillis = millis();
    calibState  = nextState;
}

// =============================================================================
// ESP-NOW helper
// =============================================================================

void Calibration::sendResult(struct_message_all msg)
{
    board.sendToLaptop(msg);
}

// =============================================================================
// Calib 1 – Marching on the spot (32 steps)
// =============================================================================

void Calibration::runCalib1(unsigned long now)
{
    switch (marchState) {
        case MARCHPREP:
            board.clearAll();
            prepCalib4();
            startMillis = now;
            exCounter   = 0;
            marchState  = MARCHSTART;
            break;

        case MARCHSTART:
            if (now - startMillis >= (unsigned long)(stepDelay * 2)) {
                clearT10T11();
                stepBackLeft();
                startMillis = now;
                marchState  = MARCH1END;
            }
            break;

        case MARCH1END:  // weight expected on T10 (tile 9)
            if (now - startMillis >= (unsigned long)stepDelay) {
                clearT10T11();
                stepBackRight();
                startMillis = now;
                marchState  = MARCH2END;
            }
            break;

        case MARCH2END:  // weight expected on T11 (tile 10)
            if (now - startMillis >= (unsigned long)stepDelay) {
                clearT10T11();
                stepBackLeft();
                exCounter++;
                if (exCounter == 32) {
                    exCounter  = 0;
                    board.clearAll();
                    marchState = MARCHDONE;
                } else {
                    startMillis = now;
                    marchState  = MARCH1END;
                }
            }
            break;

        case MARCHDONE:
            break;
    }
}

// =============================================================================
// Calib 2 – Static balance: Left / Right / Toes / Heels
// Tiles: T10 = 9, T11 = 10
// sensorValue indices: T10A=18, T10B=19, T11A=20, T11B=21
// =============================================================================

void Calibration::runCalib2(unsigned long now)
{
    switch (balState) {
        case BAL1PREP:
            board.clearAll();
            prepCalib4();
            startMillis     = now;
            balanceScore    = 0;
            balChecker      = 0;
            balanceAchieved = 0;
            balState        = BAL1START;
            break;

        case BAL1START:
            if (now - startMillis >= (unsigned long)(stepDelay * 2)) {
                clearT10T11();
                stepBackLeft();
                startMillis = now;
                balState    = BAL1END;
            }
            break;

        case BAL1END:  // balance on left (T10): toe18>on, heel19>on, T11 both <on
            if (sensorValue[18] > weightOn && sensorValue[19] > weightOn &&
                sensorValue[20] < weightOn && sensorValue[21] < weightOn) {
                balChecker++;
                balanceAchieved = balChecker / 200;
            }
            if (now - startMillis >= (unsigned long)(balanceDelay * 2)) {
                clearT10T11();
                balanceScore   += balanceAchieved;
                balChecker      = 0;
                balanceAchieved = 0;
                startMillis     = now;
                balState        = BAL2START;
            }
            break;

        case BAL2START:
            if (now - startMillis >= (unsigned long)(stepDelay * 2)) {
                stepBackRight();
                startMillis = now;
                balState    = BAL2END;
            }
            break;

        case BAL2END:  // balance on right (T11)
            if (sensorValue[18] < weightOn && sensorValue[19] < weightOn &&
                sensorValue[20] > weightOn && sensorValue[21] > weightOn) {
                balChecker++;
                balanceAchieved = balChecker / 200;
            }
            if (now - startMillis >= (unsigned long)(balanceDelay * 2)) {
                clearT10T11();
                balanceScore   += balanceAchieved;
                balChecker      = 0;
                balanceAchieved = 0;
                startMillis     = now;
                balState        = BAL3START;
            }
            break;

        case BAL3START:  // both toes – T10 TOP_RIGHT (Q2), T11 TOP_LEFT (Q1)
            if (now - startMillis >= (unsigned long)(stepDelay * 2)) {
                board.light(9,  Colours::cyan,    Tile::TOP_RIGHT);  // T10 toe
                board.light(10, Colours::magenta, Tile::TOP_LEFT);   // T11 toe
                startMillis = now;
                balState    = BAL3END;
            }
            break;

        case BAL3END:  // balance on toes: T10A>on, T11A>on, heels <on
            if (sensorValue[18] > weightOn && sensorValue[19] < weightOn &&
                sensorValue[20] > weightOn && sensorValue[21] < weightOn) {
                balChecker++;
                balanceAchieved = balChecker / 100;
            }
            if (now - startMillis >= (unsigned long)balanceDelay) {
                clearT10T11();
                balanceScore   += balanceAchieved;
                balChecker      = 0;
                balanceAchieved = 0;
                startMillis     = now;
                balState        = BAL4START;
            }
            break;

        case BAL4START:  // both heels – T10 BOTTOM_RIGHT (Q4), T11 BOTTOM_LEFT (Q3)
            if (now - startMillis >= (unsigned long)(stepDelay * 2)) {
                board.light(9,  Colours::cyan,    Tile::BOTTOM_RIGHT);  // T10 heel
                board.light(10, Colours::magenta, Tile::BOTTOM_LEFT);   // T11 heel
                startMillis = now;
                balState    = BAL4END;
            }
            break;

        case BAL4END:  // balance on heels: T10B>on, T11B>on, toes <on
            if (sensorValue[18] < weightOn && sensorValue[19] > weightOn &&
                sensorValue[20] < weightOn && sensorValue[21] > weightOn) {
                balChecker++;
                balanceAchieved = balChecker / 100;
            }
            if (now - startMillis >= (unsigned long)balanceDelay) {
                clearT10T11();
                balanceScore   += balanceAchieved;
                balanceScore   /= 4;  // average across 4 phases

                struct_message_all msg{};
                msg.id = 6;
                msg.fA = balanceScore;
                sendResult(msg);

                balChecker      = 0;
                balanceAchieved = 0;
                balanceScore    = 0;
                board.clearAll();
                balState        = BALDONE;
            }
            break;

        case BALDONE:
            break;
    }
}

// =============================================================================
// Calib 3 – Side leg lifts (5L + 5R = 10 reps)
// T9=8, T10=9, T11=10, T12=11
// sensorValue: T10A=18, T10B=19, T11A=20, T11B=21
// =============================================================================

void Calibration::runCalib3(unsigned long now)
{
    switch (sideState) {
        case SIDEPREP:
            board.clearAll();
            prepCalib4();
            startMillis      = now;
            balanceScoreSide = 0;
            balChecker       = 0;
            balanceAchieved  = 0;
            exCounter        = 0;
            sideState        = SIDE1START;
            break;

        case SIDE1START:  // left leg out – stand on T11, stripe left toward T9
            if (now - startMillis >= (unsigned long)(stepDelay * 2)) {
                clearRow3();
                // Stand-on: T11 LEFT_HALF magenta
                board.light(10, Colours::magenta, Tile::LEFT_HALF);
                // Direction indicators: T10 and T9 top stripe cyan
                board.light(9,  Colours::cyan, Tile::CENTRE_LINE_HORIZONTAL);
                board.light(8,  Colours::cyan, Tile::CENTRE_LINE_HORIZONTAL);
                startMillis = now;
                sideState   = SIDE1END;
            }
            break;

        case SIDE1END:  // balance on right leg (T11)
            if (sensorValue[18] < weightOn && sensorValue[19] < weightOn &&
                sensorValue[20] > weightOn && sensorValue[21] > weightOn) {
                balChecker++;
                balanceAchieved = balChecker / 20;
            }
            if (now - startMillis >= (unsigned long)sideliftDelay) {
                clearRow3();
                prepCalib4();
                balanceScoreSide += balanceAchieved;
                balChecker        = 0;
                balanceAchieved   = 0;
                exCounter++;
                startMillis       = now;
                sideState         = SIDE2START;
            }
            break;

        case SIDE2START:  // right leg out – stand on T10, stripe right toward T12
            if (now - startMillis >= (unsigned long)(stepDelay * 2)) {
                clearRow3();
                board.light(9,  Colours::cyan,    Tile::RIGHT_HALF);
                board.light(10, Colours::magenta, Tile::CENTRE_LINE_HORIZONTAL);
                board.light(11, Colours::magenta, Tile::CENTRE_LINE_HORIZONTAL);
                startMillis = now;
                sideState   = SIDE2END;
            }
            break;

        case SIDE2END:  // balance on left leg (T10)
            if (sensorValue[18] > weightOn && sensorValue[19] > weightOn &&
                sensorValue[20] < weightOn && sensorValue[21] < weightOn) {
                balChecker++;
                balanceAchieved = balChecker / 20;
            }
            if (now - startMillis >= (unsigned long)sideliftDelay) {
                clearRow3();
                prepCalib4();
                balanceScoreSide += balanceAchieved;
                balChecker        = 0;
                balanceAchieved   = 0;
                exCounter++;

                if (exCounter == 10) {
                    balanceScoreSide /= 10;

                    struct_message_all msg{};
                    msg.id = 6;
                    msg.dB = balanceScoreSide;
                    sendResult(msg);

                    balChecker       = 0;
                    balanceAchieved  = 0;
                    exCounter        = 0;
                    balanceScoreSide = 0;
                    board.clearAll();
                    sideState = SIDEDONE;
                } else {
                    startMillis = now;
                    sideState   = SIDE1START;
                }
            }
            break;

        case SIDEDONE:
            break;
    }
}

// =============================================================================
// Calib 4 – Clockface agility (16-tap circuit × 2 reps)
//
// sensorValue index → tile index mapping used in the original:
//   sv[9]  = T5B  → tile 4, heel
//   sv[11] = T6B  → tile 5, heel
//   sv[13] = T7B  → tile 6, heel
//   sv[15] = T8B  → tile 7, heel
//   sv[16] = T9A  → tile 8, toe
//   sv[17] = T9B  → tile 8, heel
//   sv[18] = T10A → tile 9, toe
//   sv[19] = T10B → tile 9, heel
//   sv[20] = T11A → tile 10, toe
//   sv[21] = T11B → tile 10, heel
//   sv[22] = T12A → tile 11, toe
//   sv[23] = T12B → tile 11, heel
//   sv[24] = T13A → tile 12, toe
//   sv[26] = T14A → tile 13, toe
//   sv[27] = T14B → tile 13, heel
//   sv[28] = T15A → tile 14, toe
//   sv[29] = T15B → tile 14, heel
//   sv[30] = T16A → tile 15, toe
// =============================================================================

void Calibration::runCalib4(unsigned long now)
{
    switch (calibState) {
        // ---- Prep: light T10/T11, wait stepDelaymob then switch on first tap tile ----
        case STEP1_PREP:
            prepCalib4();
            startMillis = now;
            calibState  = STEP1_WAIT_PREP_OFF_LIGHTS_ON;
            break;

        case STEP1_WAIT_PREP_OFF_LIGHTS_ON:
            if (now - startMillis >= (unsigned long)stepDelaymob) {
                clearT10();
                lightTapTile(5, Tile::BOTTOM_RIGHT, Colours::cyan);  // T6 Q4 (pos5)
                startMillis = now;
                calibState  = STEP1_LIGHTS_ON_WAIT_TAP;
            }
            break;

        // ---- Steps 1-8: left-side sweep, stepback = T10 (left) ----

        case STEP1_LIGHTS_ON_WAIT_TAP:   // T6 Q4 → sensorValue[11] (T6B)
            processTap(11);
            if (now - startMillis >= (unsigned long)stepDelaymob)
                finishTapStep(5, true, STEP1_STEPBACK_LIGHTS2_ON);
            break;

        case STEP1_STEPBACK_LIGHTS2_ON:
            if (now - startMillis >= (unsigned long)stepDelaymob) {
                clearT10();
                lightTapTile(5, Tile::BOTTOM_LEFT, Colours::cyan);   // T6 Q3 (pos4)
                startMillis = now;
                calibState  = STEP2_LIGHTS_ON_WAIT_TAP;
            }
            break;

        case STEP2_LIGHTS_ON_WAIT_TAP:   // T6 Q3 → sensorValue[11] (T6B)
            processTap(11);
            if (now - startMillis >= (unsigned long)stepDelaymob)
                finishTapStep(5, true, STEP2_STEPBACK_LIGHTS3_ON);
            break;

        case STEP2_STEPBACK_LIGHTS3_ON:
            if (now - startMillis >= (unsigned long)stepDelaymob) {
                clearT10();
                lightTapTile(4, Tile::BOTTOM_RIGHT, Colours::cyan);  // T5 Q4 (pos5)
                startMillis = now;
                calibState  = STEP3_LIGHTS_ON_WAIT_TAP;
            }
            break;

        case STEP3_LIGHTS_ON_WAIT_TAP:   // T5 Q4 → sensorValue[9] (T5B)
            processTap(9);
            if (now - startMillis >= (unsigned long)stepDelaymob)
                finishTapStep(4, true, STEP3_STEPBACK_LIGHTS4_ON);
            break;

        case STEP3_STEPBACK_LIGHTS4_ON:
            if (now - startMillis >= (unsigned long)stepDelaymob) {
                clearT10();
                lightTapTile(8, Tile::RIGHT_HALF, Colours::cyan);    // T9 righthalftile (pos0)
                startMillis = now;
                calibState  = STEP4_LIGHTS_ON_WAIT_TAP;
            }
            break;

        case STEP4_LIGHTS_ON_WAIT_TAP:   // T9 → average sensorValue[16]+[17]
            processTapAvg(16, 17);
            if (now - startMillis >= (unsigned long)stepDelaymob)
                finishTapStep(8, true, STEP4_STEPBACK_LIGHTS5_ON);
            break;

        case STEP4_STEPBACK_LIGHTS5_ON:
            if (now - startMillis >= (unsigned long)stepDelaymob) {
                clearT10();
                lightTapTile(12, Tile::TOP_RIGHT, Colours::cyan);    // T13 Q2 (pos3)
                startMillis = now;
                calibState  = STEP5_LIGHTS_ON_WAIT_TAP;
            }
            break;

        case STEP5_LIGHTS_ON_WAIT_TAP:   // T13 → sensorValue[24] (T13A)
            processTap(24);
            if (now - startMillis >= (unsigned long)stepDelaymob)
                finishTapStep(12, true, STEP5_STEPBACK_LIGHTS6_ON);
            break;

        case STEP5_STEPBACK_LIGHTS6_ON:
            if (now - startMillis >= (unsigned long)stepDelaymob) {
                clearT10();
                lightTapTile(13, Tile::LEFT_HALF, Colours::cyan);    // T14 lefthalftile (pos1)
                startMillis = now;
                calibState  = STEP6_LIGHTS_ON_WAIT_TAP;
            }
            break;

        case STEP6_LIGHTS_ON_WAIT_TAP:   // T14 → average sensorValue[26]+[27]
            processTapAvg(26, 27);
            if (now - startMillis >= (unsigned long)stepDelaymob)
                finishTapStep(13, true, STEP6_STEPBACK_LIGHTS7_ON);
            break;

        case STEP6_STEPBACK_LIGHTS7_ON:
            if (now - startMillis >= (unsigned long)stepDelaymob) {
                clearT10();
                lightTapTile(6, Tile::LEFT_HALF, Colours::cyan);     // T7 lefthalftile (pos1)
                startMillis = now;
                calibState  = STEP7_LIGHTS_ON_WAIT_TAP;
            }
            break;

        case STEP7_LIGHTS_ON_WAIT_TAP:   // T7 → sensorValue[13] (T7B), 2× delay
            processTap(13);
            if (now - startMillis >= (unsigned long)(stepDelaymob * 2))
                finishTapStep(6, true, STEP7_STEPBACK_LIGHTS8_ON);
            break;

        case STEP7_STEPBACK_LIGHTS8_ON:
            if (now - startMillis >= (unsigned long)stepDelaymob) {
                clearT10();
                lightTapTile(9, Tile::RIGHT_HALF, Colours::cyan);    // T10 righthalftile (pos0)
                startMillis = now;
                calibState  = STEP8_LIGHTS_ON_WAIT_TAP;
            }
            break;

        // ---- Step 8: HALFWAY – pivot from left stepback to right stepback ----
        case STEP8_LIGHTS_ON_WAIT_TAP:   // T10 → average sensorValue[18]+[19], 2× delay
            processTapAvg(18, 19);
            if (now - startMillis >= (unsigned long)(stepDelaymob * 2)) {
                clearTile(10);  // T11 (0-based)
                maxcount += tapcounter;
                Serial.print(", total score: ");
                Serial.println(maxcount);
                mobScore++;
                tapcounter = 0;
                // Step 9: T7 Q3, right colour, right stepback
                lightTapTile(6, Tile::BOTTOM_LEFT, Colours::magenta); // T7 Q3 (pos4)
                startMillis = now;
                calibState  = STEP9_LIGHTS_ON_WAIT_TAP;
            }
            break;

        // ---- Steps 9-16: right-side sweep, stepback = T11 (right) ----

        case STEP9_LIGHTS_ON_WAIT_TAP:   // T7 → sensorValue[13] (T7B)
            processTap(13);
            if (now - startMillis >= (unsigned long)stepDelaymob)
                finishTapStep(6, false, STEP9_STEPBACK_LIGHTS10_ON);
            break;

        case STEP9_STEPBACK_LIGHTS10_ON:
            if (now - startMillis >= (unsigned long)stepDelaymob) {
                clearT11();
                lightTapTile(6, Tile::BOTTOM_RIGHT, Colours::magenta); // T7 Q4 (pos5)
                startMillis = now;
                calibState  = STEP10_LIGHTS_ON_WAIT_TAP;
            }
            break;

        case STEP10_LIGHTS_ON_WAIT_TAP:  // T7 → sensorValue[13] (T7B)
            processTap(13);
            if (now - startMillis >= (unsigned long)stepDelaymob)
                finishTapStep(6, false, STEP10_STEPBACK_LIGHTS11_ON);
            break;

        case STEP10_STEPBACK_LIGHTS11_ON:
            if (now - startMillis >= (unsigned long)stepDelaymob) {
                clearT11();
                lightTapTile(7, Tile::BOTTOM_LEFT, Colours::magenta); // T8 Q3 (pos4)
                startMillis = now;
                calibState  = STEP11_LIGHTS_ON_WAIT_TAP;
            }
            break;

        case STEP11_LIGHTS_ON_WAIT_TAP:  // T8 → sensorValue[15] (T8B)
            processTap(15);
            if (now - startMillis >= (unsigned long)stepDelaymob)
                finishTapStep(7, false, STEP11_STEPBACK_LIGHTS12_ON);
            break;

        case STEP11_STEPBACK_LIGHTS12_ON:
            if (now - startMillis >= (unsigned long)stepDelaymob) {
                clearT11();
                lightTapTile(11, Tile::LEFT_HALF, Colours::magenta);  // T12 lefthalftile (pos1)
                startMillis = now;
                calibState  = STEP12_LIGHTS_ON_WAIT_TAP;
            }
            break;

        case STEP12_LIGHTS_ON_WAIT_TAP:  // T12 → average sensorValue[22]+[23]
            processTapAvg(22, 23);
            if (now - startMillis >= (unsigned long)stepDelaymob)
                finishTapStep(11, false, STEP12_STEPBACK_LIGHTS13_ON);
            break;

        case STEP12_STEPBACK_LIGHTS13_ON:
            if (now - startMillis >= (unsigned long)stepDelaymob) {
                clearT11();
                lightTapTile(15, Tile::TOP_LEFT, Colours::magenta);   // T16 Q1 (pos2)
                startMillis = now;
                calibState  = STEP13_LIGHTS_ON_WAIT_TAP;
            }
            break;

        case STEP13_LIGHTS_ON_WAIT_TAP:  // T16 → sensorValue[30] (T16A)
            processTap(30);
            if (now - startMillis >= (unsigned long)stepDelaymob)
                finishTapStep(15, false, STEP13_STEPBACK_LIGHTS14_ON);
            break;

        case STEP13_STEPBACK_LIGHTS14_ON:
            if (now - startMillis >= (unsigned long)stepDelaymob) {
                clearT11();
                lightTapTile(14, Tile::RIGHT_HALF, Colours::magenta); // T15 righthalftile (pos0)
                startMillis = now;
                calibState  = STEP14_LIGHTS_ON_WAIT_TAP;
            }
            break;

        case STEP14_LIGHTS_ON_WAIT_TAP:  // T15 → average sensorValue[28]+[29]
            processTapAvg(28, 29);
            if (now - startMillis >= (unsigned long)stepDelaymob)
                finishTapStep(14, false, STEP14_STEPBACK_LIGHTS15_ON);
            break;

        case STEP14_STEPBACK_LIGHTS15_ON:
            if (now - startMillis >= (unsigned long)stepDelaymob) {
                clearT11();
                lightTapTile(5, Tile::RIGHT_HALF, Colours::magenta);  // T6 righthalftile (pos0)
                startMillis = now;
                calibState  = STEP15_LIGHTS_ON_WAIT_TAP;
            }
            break;

        case STEP15_LIGHTS_ON_WAIT_TAP:  // T6 → average sensorValue[10]+[11], 2× delay
            processTapAvg(10, 11);
            if (now - startMillis >= (unsigned long)(stepDelaymob * 2))
                finishTapStep(5, false, STEP15_STEPBACK_LIGHTS16_ON);
            break;

        case STEP15_STEPBACK_LIGHTS16_ON:
            if (now - startMillis >= (unsigned long)(stepDelaymob * 2)) {
                clearT11();
                lightTapTile(10, Tile::LEFT_HALF, Colours::magenta);  // T11 lefthalftile (pos1)
                startMillis = now;
                calibState  = STEP16_LIGHTS_ON_WAIT_TAP;
            }
            break;

        case STEP16_LIGHTS_ON_WAIT_TAP:  // T11 → average sensorValue[20]+[21], 2× delay
            processTapAvg(20, 21);
            if (now - startMillis >= (unsigned long)(stepDelaymob * 2)) {
                maxcount += tapcounter;
                Serial.print(", total score: ");
                Serial.println(maxcount);
                mobScore++;
                tapcounter = 0;
                exCounter++;

                if (exCounter == 2) {
                    // Two reps complete – calculate and send score
                    mobScore = (int)(mobScore / 0.32f);

                    struct_message_all msg{};
                    msg.id = 6;
                    msg.dA = mobScore;
                    msg.eB = maxcount;
                    sendResult(msg);

                    exCounter  = 0;
                    maxcount   = 0;
                    mobScore   = 0;
                    board.clearAll();
                    calibState = STEP16_DONE;
                } else {
                    startMillis = now;
                    calibState  = STEP1_WAIT_PREP_OFF_LIGHTS_ON;  // second rep
                }
            }
            break;

        case STEP16_DONE:
            break;
    }
}

// =============================================================================
// Calib 5 – Alternating lunges (5L + 5R = 10 reps)
// T6=5, T7=6, T10=9, T11=10, T14=13, T15=14
// sensorValue: T6A=10, T6B=11, T7A=12, T7B=13
// =============================================================================

void Calibration::runCalib5(unsigned long now)
{
    switch (lungeState) {
        case LUNGEPREP:
            board.clearAll();
            // T6 RIGHT_HALF cyan (stand left), T7 LEFT_HALF magenta (stand right)
            board.light(5, Colours::cyan,    Tile::RIGHT_HALF);
            board.light(6, Colours::magenta, Tile::LEFT_HALF);
            startMillis     = now;
            balanceScoreDyn = 0;
            balChecker      = 0;
            balanceAchieved = 0;
            exCounter       = 0;
            lungeState      = LUNGE1START;
            break;

        case LUNGE1START:  // left foot back – stand on T7 (right), show lunge lines
            if (now - startMillis >= (unsigned long)(stepDelay * 2)) {
                // Clear T6/T7 standing lights, show lunge pattern
                board.clear(5);
                board.clear(6);
                // Stand on T7 (right), lunge lines on T10 and T14 (centre strip)
                board.light(6,  Colours::magenta, Tile::LEFT_HALF);
                board.light(9,  Colours::cyan,    Tile::CENTRE_LINE_VERTICAL);
                board.light(13, Colours::cyan,    Tile::CENTRE_LINE_VERTICAL);
                startMillis = now;
                lungeState  = LUNGE1END;
            }
            break;

        case LUNGE1END:  // balance on right leg (T7): T6 off, T7 on
            if (sensorValue[10] < weightOn && sensorValue[11] < weightOn &&
                sensorValue[12] > weightOn && sensorValue[13] > weightOn) {
                balChecker++;
                balanceAchieved = balChecker / 20;
            }
            if (now - startMillis >= (unsigned long)lungeDelay) {
                clearLungeTiles();
                board.light(5, Colours::cyan,    Tile::RIGHT_HALF);
                board.light(6, Colours::magenta, Tile::LEFT_HALF);
                balanceScoreDyn += balanceAchieved;
                balChecker       = 0;
                balanceAchieved  = 0;
                exCounter++;
                startMillis      = now;
                lungeState       = LUNGE2START;
            }
            break;

        case LUNGE2START:  // right foot back – stand on T6 (left), show lunge lines
            if (now - startMillis >= (unsigned long)(stepDelay * 2)) {
                board.clear(5);
                board.clear(6);
                board.light(5,  Colours::cyan,    Tile::RIGHT_HALF);
                board.light(10, Colours::magenta, Tile::CENTRE_LINE_VERTICAL);
                board.light(14, Colours::magenta, Tile::CENTRE_LINE_VERTICAL);
                startMillis = now;
                lungeState  = LUNGE2END;
            }
            break;

        case LUNGE2END:  // balance on left leg (T6)
            if (sensorValue[10] > weightOn && sensorValue[11] > weightOn &&
                sensorValue[12] < weightOn && sensorValue[13] < weightOn) {
                balChecker++;
                balanceAchieved = balChecker / 20;
            }
            if (now - startMillis >= (unsigned long)lungeDelay) {
                clearLungeTiles();
                board.light(5, Colours::cyan,    Tile::RIGHT_HALF);
                board.light(6, Colours::magenta, Tile::LEFT_HALF);
                balanceScoreDyn += balanceAchieved;
                exCounter++;

                if (exCounter == 10) {
                    balanceScoreDyn /= 10;

                    struct_message_all msg{};
                    msg.id = 6;
                    msg.fB = balanceScoreDyn;
                    sendResult(msg);

                    balChecker      = 0;
                    balanceAchieved = 0;
                    exCounter       = 0;
                    balanceScoreDyn = 0;
                    board.clearAll();
                    lungeState = LUNGEDONE;
                } else {
                    balChecker      = 0;
                    balanceAchieved = 0;
                    startMillis     = now;
                    lungeState      = LUNGE1START;
                }
            }
            break;

        case LUNGEDONE:
            break;
    }
}

// =============================================================================
// Calib 6 – Squats: 3 narrow (squat1Delay) + 3 wide-pulse (squat2Delay)
// T10=9, T11=10
// sensorValue: T10A=18, T10B=19, T11A=20, T11B=21
// =============================================================================

void Calibration::runCalib6(unsigned long now)
{
    // All squat sensor checks: all four sensors on T10/T11 must be > weightOn
    auto allOn = [&]() {
        return sensorValue[18] > weightOn && sensorValue[19] > weightOn &&
               sensorValue[20] > weightOn && sensorValue[21] > weightOn;
    };

    switch (squatState) {
        case SQUATPREP:
            prepCalib4();
            startMillis = now;
            squatScore  = 0;
            squatState  = SQUAT1START;
            break;

        // ---- Narrow squats 1-3 ----
        case SQUAT1START:
            if (now - startMillis >= (unsigned long)(stepDelay * 4)) {
                clearT10T11();
                // Narrow stand: T10 RIGHT_HALF white, T11 LEFT_HALF white
                board.light(9,  Colours::white, Tile::RIGHT_HALF);
                board.light(10, Colours::white, Tile::LEFT_HALF);
                startMillis = now;
                squatState  = SQUAT1END;
            }
            break;
        case SQUAT1END:
            squatCounter = 0;
            if (allOn()) { squatCounter++; squatScore = 100; }
            if (now - startMillis >= (unsigned long)squat1Delay) {
                clearT10T11();
                strengthScore += squatScore;
                squatScore     = 0;
                startMillis    = now;
                squatState     = SQUAT2START;
            }
            break;

        case SQUAT2START:
            if (now - startMillis >= (unsigned long)(stepDelay * 4)) {
                board.light(9,  Colours::white, Tile::RIGHT_HALF);
                board.light(10, Colours::white, Tile::LEFT_HALF);
                startMillis = now;
                squatState  = SQUAT2END;
            }
            break;
        case SQUAT2END:
            squatCounter = 0;
            if (allOn()) { squatCounter++; squatScore = 100; }
            if (now - startMillis >= (unsigned long)squat1Delay) {
                clearT10T11();
                strengthScore += squatScore;
                squatScore     = 0;
                startMillis    = now;
                squatState     = SQUAT3START;
            }
            break;

        case SQUAT3START:
            if (now - startMillis >= (unsigned long)(stepDelay * 4)) {
                board.light(9,  Colours::white, Tile::RIGHT_HALF);
                board.light(10, Colours::white, Tile::LEFT_HALF);
                startMillis = now;
                squatState  = SQUAT3END;
            }
            break;
        case SQUAT3END:
            squatCounter = 0;
            if (allOn()) { squatCounter++; squatScore = 100; }
            if (now - startMillis >= (unsigned long)squat1Delay) {
                clearT10T11();
                strengthScore += squatScore;
                squatScore     = 0;
                startMillis    = now;
                squatState     = SQUAT4PREP;
            }
            break;

        // ---- Wide squats 4-6 ----
        case SQUAT4PREP:
            // Wide prep: T10 LEFT_HALF cyan, T11 RIGHT_HALF magenta
            board.light(9,  Colours::cyan,    Tile::LEFT_HALF);
            board.light(10, Colours::magenta, Tile::RIGHT_HALF);
            startMillis = now;
            squatState  = SQUAT4START;
            break;

        case SQUAT4START:
            if (now - startMillis >= (unsigned long)(stepDelay * 4)) {
                clearT10T11();
                board.light(9,  Colours::white, Tile::LEFT_HALF);
                board.light(10, Colours::white, Tile::RIGHT_HALF);
                startMillis = now;
                squatState  = SQUAT4END;
            }
            break;
        case SQUAT4END:
            squatCounter = 0;
            if (allOn()) { squatCounter++; squatScore = 100; }
            if (now - startMillis >= (unsigned long)squat2Delay) {
                clearT10T11();
                strengthScore += squatScore;
                squatScore     = 0;
                startMillis    = now;
                squatState     = SQUAT5START;
            }
            break;

        case SQUAT5START:
            if (now - startMillis >= (unsigned long)(stepDelay * 4)) {
                board.light(9,  Colours::white, Tile::LEFT_HALF);
                board.light(10, Colours::white, Tile::RIGHT_HALF);
                startMillis = now;
                squatState  = SQUAT5END;
            }
            break;
        case SQUAT5END:
            squatCounter = 0;
            if (allOn()) { squatCounter++; squatScore = 100; }
            if (now - startMillis >= (unsigned long)squat2Delay) {
                clearT10T11();
                strengthScore += squatScore;
                squatScore     = 0;
                startMillis    = now;
                squatState     = SQUAT6START;
            }
            break;

        case SQUAT6START:
            if (now - startMillis >= (unsigned long)(stepDelay * 4)) {
                board.light(9,  Colours::white, Tile::LEFT_HALF);
                board.light(10, Colours::white, Tile::RIGHT_HALF);
                startMillis = now;
                squatState  = SQUAT6END;
            }
            break;
        case SQUAT6END:
            squatCounter = 0;
            if (allOn()) { squatCounter++; squatScore = 100; }
            if (now - startMillis >= (unsigned long)squat2Delay) {
                clearT10T11();
                strengthScore += squatScore;
                strengthScore  /= 6;

                struct_message_all msg{};
                msg.id = 6;
                msg.gA = strengthScore;
                sendResult(msg);

                squatCounter  = 0;
                squatScore    = 0;
                strengthScore = 0;
                board.clearAll();
                squatState = SQUATDONE;
            }
            break;

        case SQUATDONE:
            break;
    }
}

// =============================================================================
// Calib 7 – Airtime jump × 3
// Jumping tiles: T14=13, T15=14
// sensorValue: T14A=26, T14B=27, T15A=28, T15B=29
// =============================================================================

void Calibration::runCalib7(unsigned long now)
{
    // Any jumping-tile sensor active?
    auto anyOnTiles = [&]() {
        return sensorValue[26] > weightOn || sensorValue[27] > weightOn ||
               sensorValue[28] > weightOn || sensorValue[29] > weightOn;
    };
    auto allOffTiles = [&]() {
        return sensorValue[26] < weightOn && sensorValue[27] < weightOn &&
               sensorValue[28] < weightOn && sensorValue[29] < weightOn;
    };

    switch (jumpState) {
        case JUMPPREP:
            board.clearAll();
            board.light(13, Colours::blue, Tile::OUTLINE);
            board.light(14, Colours::blue, Tile::OUTLINE);
            startMillis = now;
            exCounter   = 0;
            jumpState   = JUMPBENCHMARK;
            break;

        case JUMPBENCHMARK:  // 5 s standing balance
            if (anyOnTiles()) {
                struct_message_all msg{};
                msg.id = 6;
                msg.gB = 111;  // person on tile
                sendResult(msg);
            }
            if (now - startMillis >= (unsigned long)(stepDelay * 10)) {
                board.clear(13);
                board.clear(14);
                struct_message_all msg{};
                msg.id = 6;
                msg.gB = 0;
                sendResult(msg);
                startMillis = now;
                jumpState   = JUMPSTART;
            }
            break;

        case JUMPSTART:  // 2 s dark, then orange = jump cue
            if (now - startMillis >= (unsigned long)(stepDelay * 4)) {
                board.light(13, Colours::orange, Tile::OUTLINE);
                board.light(14, Colours::orange, Tile::OUTLINE);
                struct_message_all msg{};
                msg.id = 6;
                msg.gB = 222;  // jump lights started
                sendResult(msg);
                startMillis = now;
                jumpState   = JUMPOFF;
            }
            break;

        case JUMPOFF:  // sample take-off; wait airtime*2
            if (allOffTiles()) {
                struct_message_all msg{};
                msg.id = 6;
                msg.gB = 333;  // in the air
                sendResult(msg);
            }
            if (now - startMillis >= (unsigned long)(airtimeDefault * 2)) {
                board.clear(13);
                board.clear(14);
                startMillis = now;
                jumpState   = JUMPLANDPREP;
            }
            break;

        case JUMPLANDPREP:  // short gap before landing light
            if (allOffTiles()) {
                struct_message_all msg{};
                msg.id = 6; msg.gB = 333;
                sendResult(msg);
            } else if (anyOnTiles()) {
                struct_message_all msg{};
                msg.id = 6; msg.gB = 444;
                sendResult(msg);
            }
            if (now - startMillis >= (unsigned long)(stepDelay / 2)) {
                board.light(13, Colours::blue, Tile::OUTLINE);
                board.light(14, Colours::blue, Tile::OUTLINE);
                startMillis = now;
                jumpState   = JUMPLAND;
            }
            break;

        case JUMPLAND:  // 3.5 s landing balance, blue lights
            if (anyOnTiles()) {
                struct_message_all msg{};
                msg.id = 6; msg.gB = 444;
                sendResult(msg);
            }
            if (now - startMillis >= (unsigned long)(stepDelay * 7)) {
                board.clear(13);
                board.clear(14);
                struct_message_all msg{};
                msg.id = 6; msg.gB = 0;
                sendResult(msg);
                startMillis = now;
                jumpState   = JUMPEND;
            }
            break;

        case JUMPEND:  // 2 s rest between jumps
            if (now - startMillis >= (unsigned long)(stepDelay * 4)) {
                exCounter++;
                if (exCounter == 3) {
                    board.clearAll();
                    exCounter = 0;
                    jumpState = JUMPDONE;
                } else {
                    startMillis = now;
                    jumpState   = JUMPSTART;
                }
            }
            break;

        case JUMPDONE:
            break;
    }
}

// =============================================================================
// Calibend – flash all tiles purple then hold
// =============================================================================

void Calibration::runCalibEnd(unsigned long now)
{
    switch (finalState) {
        case FINAL:
            // Kick off the sequential tile flash
            board.clearAll();
            flashIdx    = 0;
            startMillis = now;
            finalState  = FINAL_FLASH;
            break;

        case FINAL_FLASH:
            // Flash each tile in turn, one per (stepDelay/8) ms
            if (now - startMillis >= (unsigned long)(stepDelay / 8)) {
                board.clear(flashIdx);
                flashIdx++;
                if (flashIdx >= 16) {
                    // All tiles flashed — light all purple and start hold timer
                    board.lightAll(Colours::purple);
                    startMillis = now;
                    finalState  = FINAL_HOLD;
                } else {
                    board.light(flashIdx, Colours::purple, Tile::WHOLE);
                    startMillis = now;
                }
            }
            break;

        case FINAL_HOLD:
            // Hold the full purple board for stepDelay*6 ms, then clear
            if (now - startMillis >= (unsigned long)(stepDelay * 6)) {
                board.clearAll();
                finalState = FINALDONE;
            }
            break;

        case FINALDONE:
            break;
    }
}
