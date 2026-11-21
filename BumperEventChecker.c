// Bumper Event Checker source file

#include "ES_Configure.h"
#include "ES_Framework.h"
#include "BumperEventChecker.h"
#include "IO_Ports.h"
#include "TopHSM.h"

/*******************************************************************************
 * MODULE #DEFINES                                                             *
 ******************************************************************************/

// pinout
#define FL_PORT PORTY
#define FL_PIN PIN11
#define FR_PORT PORTZ
#define FR_PIN PIN11
#define RL_PORT PORTY
#define RL_PIN PIN8
#define RR_PORT PORTY
#define RR_PIN PIN9

#define BUMP_DEBOUNCE_MS 10 // 10 ms debounce

/*******************************************************************************
 * PRIVATE MODULE VARIABLES                                                    *
 ******************************************************************************/

static uint8_t candidateBits = 0; // latest raw
static uint32_t candidateTime = 0; // ms from when it last changed
static uint8_t stableBits = 0;    // latest debounced reading

/*******************************************************************************
 * PUBLIC FUNCTIONS                                                            *
 ******************************************************************************/

void Bumper_Init(void)
{
    IO_PortsSetPortInputs(PORTY, FL_PIN | RL_PIN | RR_PIN);
    IO_PortsSetPortInputs(PORTZ, FR_PIN);

    // set to raw init otherwise aa bump evvent is always posted
    stableBits = Bumper_GetRaw();
    candidateBits = stableBits;
    candidateTime = 0;
}

uint8_t Bumper_GetRaw(void)
{
    uint8_t bits = 0;

    // since pullup resistor, pressed reads 0V, read the pin and send to bits
    if (!(IO_PortsReadPort(FL_PORT) & FL_PIN)){
        bits |= BUMP_FL;
    } 
    if (!(IO_PortsReadPort(FR_PORT) & FR_PIN)){
        bits |= BUMP_FR;
    } 
    if (!(IO_PortsReadPort(RL_PORT) & RL_PIN)){
        bits |= BUMP_RL;
    } 
    if (!(IO_PortsReadPort(RR_PORT) & RR_PIN)){
        bits |= BUMP_RR;
    } 

    return bits;
}

uint8_t Bumper_GetBits(void)
{
    return stableBits;
}

uint8_t CheckBumpers(void)
{
    ES_Event thisEvent;
    
    // get raw readings
    uint8_t raw = Bumper_GetRaw();
    // get wall clock
    uint32_t now = ES_Timer_GetTime();

    if (raw != candidateBits) {
        // still bouncing (or a new press), restart the clock
        candidateBits = raw;
        candidateTime = now;
        return FALSE;
    }

    // if more than 10 ms have passed and candidate bits are different, then post an event with the bitmask
    if (candidateBits != stableBits && (now - candidateTime) >= BUMP_DEBOUNCE_MS) {
        // held steady long enough, post it
        thisEvent.EventType = BUMPER_CHANGED;
        // pass the latest new reading and the previous reading, so the state machine can take difference
        thisEvent.EventParam = ((uint16_t) (candidateBits ^ stableBits) << 8) | candidateBits;
        stableBits = candidateBits;
        PostTopHSM(thisEvent);
        return TRUE;
    }

    return FALSE;
}
