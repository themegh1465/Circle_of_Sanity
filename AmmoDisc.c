#include "ES_Configure.h"
#include "ES_Framework.h"
#include "BOARD.h"
#include "Stepper.h"
#include "TopHSM.h"
#include "AmmoDisc.h"

static uint8_t chamber = 0;
static uint8_t chamberMove = FALSE; // TRUE while a Ammo_NextChamber move is running
static uint8_t wasStepping = FALSE; // last thing CheckAmmoDisc saw, to catch the end of a move

// step count from chamber 0 to chamber n, rounded to the nearest whole step
static int32_t ChamberToSteps(uint8_t n)
{
    return ((int32_t) n * AMMO_STEPS_PER_REV + AMMO_CHAMBERS / 2) / AMMO_CHAMBERS;
}

void Ammo_Init(void)
{
    Stepper_Init();
    Stepper_SetRate(AMMO_STEP_RATE);
    chamber = 0;
    chamberMove = FALSE;
    wasStepping = FALSE;
}

uint8_t Ammo_NextChamber(void)
{
    // 29, 28, 29, 28, 29, 28, 29 = 200 steps for a full lap of 7
    int32_t steps = ChamberToSteps(chamber + 1) - ChamberToSteps(chamber);

    if (Stepper_IsStepping()) {
        return FALSE;
    }
    if (Stepper_InitSteps(FORWARD, steps) == ERROR) {
        return FALSE;
    }
    chamberMove = TRUE;
    return TRUE;
}

uint8_t Ammo_GetChamber(void)
{
    return chamber;
}

uint8_t CheckAmmoDisc(void)
{
    ES_Event thisEvent;
    uint8_t stepping = Stepper_IsStepping() ? TRUE : FALSE;
    uint8_t returnVal = FALSE;

    if (wasStepping && !stepping) { // a move just finished
        if (chamberMove) {
            chamber = (chamber + 1) % AMMO_CHAMBERS;
            chamberMove = FALSE;
        }
        thisEvent.EventType = STEPPER_DONE;
        thisEvent.EventParam = chamber;
        PostTopHSM(thisEvent);
        returnVal = TRUE;
    }
    wasStepping = stepping;
    return returnVal;
}
