#include "ES_Configure.h"
#include "ES_Framework.h"
#include "BannerEventChecker.h"
#include "AD.h"
#include "TopHSM.h"

/*******************************************************************************
 * MODULE #DEFINES                                                             *
 ******************************************************************************/

#define BANNER_PIN AD_PORTW3

#define BANNER_ON 700
#define BANNER_OFF 300

// 1 if the sensor reads high over the reflective banner, 0 if it reads low (flip if backwards)
#define BANNER_IS_HIGH 1

#define BANNER_DEBOUNCE_MS 5 // new state has to hold this long

/*******************************************************************************
 * PRIVATE MODULE VARIABLES                                                    *
 ******************************************************************************/

static uint8_t onBanner = FALSE;      // hysteresis output, before debounce
static uint8_t candidate = FALSE;     // latest hysteresis output
static uint32_t candidateTime = 0;    // ms from when it last changed
static uint8_t stable = FALSE;        // latest debounced state

/*******************************************************************************
 * PRIVATE FUNCTIONS                                                           *
 ******************************************************************************/

// raw reading through the hysteresis band, TRUE = on the banner
static uint8_t ReadHysteresis(void)
{
    uint16_t input = Banner_GetRaw();

    if (input > 1023) {
        return onBanner; // AD not ready yet (ERROR), keep the last state
    }
    if (!BANNER_IS_HIGH) {
        input = 1023 - input;
    }
    if (!onBanner && input > BANNER_ON) {
        onBanner = TRUE;
    } else if (onBanner && input < BANNER_OFF) {
        onBanner = FALSE;
    }
    return onBanner;
}

/*******************************************************************************
 * PUBLIC FUNCTIONS                                                            *
 ******************************************************************************/

void Banner_Init(void)
{
    AD_AddPins(BANNER_PIN);
    onBanner = FALSE;
    candidate = FALSE;
    candidateTime = 0;
    stable = FALSE; // if we start on a banner the first check posts it, which is fine
}

uint16_t Banner_GetRaw(void)
{
    return (uint16_t) AD_ReadADPin(BANNER_PIN);
}

uint8_t Banner_IsOn(void)
{
    return stable;
}

uint8_t CheckBanner(void)
{
    ES_Event thisEvent;
    uint8_t now_on = ReadHysteresis();
    uint32_t now = ES_Timer_GetTime();

    if (now_on != candidate) {
        // just changed (or flickering), restart the clock
        candidate = now_on;
        candidateTime = now;
        return FALSE;
    }

    if (candidate != stable && (now - candidateTime) >= BANNER_DEBOUNCE_MS) {
        // held steady long enough, post it
        stable = candidate;
        thisEvent.EventType = BANNER_CHANGED;
        thisEvent.EventParam = stable;
        PostTopHSM(thisEvent);
        return TRUE;
    }

    return FALSE;
}
