/*
 * File:   TapeEventChecker.c
 * Author: Gabriel Hugh Elkaim
 *
 * Created on September 27, 2013, 8:37 AM
 */

/*******************************************************************************
 * MODULE #INCLUDE                                                             *
 ******************************************************************************/

#include "ES_Configure.h"
#include "TapeEventChecker.h"
#include "ES_Events.h"
#include "serial.h"
#include "AD.h"

#ifndef EVENTCHECKER_TEST
#include "TapeService.h"
#endif

/*******************************************************************************
 * MODULE #DEFINES                                                             *
 ******************************************************************************/
#define BATTERY_DISCONNECT_THRESHOLD 175

// hysteresis bounds, the tape sensors output 1023 - 30 mostly due to the comparator but this is a safe to have. 
#define TAPE_ON 700
#define TAPE_OFF 300

// invert if tape sensors is low over tape
#define TAPE_IS_HIGH 1

#define NUM_TAPE 6

/*******************************************************************************
 * EVENTCHECKER_TEST SPECIFIC CODE                                                             *
 ******************************************************************************/

//#define EVENTCHECKER_TEST
#ifdef EVENTCHECKER_TEST
#include <stdio.h>
#define SaveEvent(x) do {eventName=__func__; storedEvent=x;} while (0)

static const char *eventName;
static ES_Event storedEvent;
#endif

/*******************************************************************************
 * PRIVATE FUNCTION PROTOTYPES                                                 *
 ******************************************************************************/
/* Prototypes for private functions for this EventChecker. They should be functions
   relevant to the behavior of this particular event checker */



/*******************************************************************************
 * PRIVATE MODULE VARIABLES                                                    *
 ******************************************************************************/

/* Any private module level variable that you might need for keeping track of
   events would be placed here. Private variables should be STATIC so that they
   are limited in scope to this module. */


// setup the pins, ncce to have them bunched together
static const uint16_t tapePins[NUM_TAPE] = {
  AD_PORTV3, AD_PORTV4, AD_PORTV5, AD_PORTV6, AD_PORTV7, AD_PORTV8  
};

// save the result of each tape sensor and last result
static uint8_t onTape[NUM_TAPE];
static uint8_t lastBits;


/*******************************************************************************
 * PUBLIC FUNCTIONS                                                            *
 ******************************************************************************/


//init func
void TapeEventChecker_Init(void){
    AD_AddPins(AD_PORTV3 | AD_PORTV4 | AD_PORTV5 | AD_PORTV6 | AD_PORTV7 | AD_PORTV8);
    
    for (uint8_t i = 0; i < NUM_TAPE; i++) {
        onTape[i] = FALSE;
    }
    lastBits = 0;
    
    // assumes that all tape sensors start off the tape, if on the tape on init it fires an event immediately
}

// convert reading with hysteresis from raw ADC to bitmask
uint8_t Tape_GetBits(void){
    
    uint8_t result = 0;
    uint16_t input;
    
    for (uint8_t i = 0; i < NUM_TAPE; i++){
        input = AD_ReadADPin(tapePins[i]);
        if(!TAPE_IS_HIGH){
            input = 1023- input;
        }
        
        if (!onTape[i] && input > TAPE_ON){
            onTape[i] = TRUE;
        } else if (onTape[i] && input < TAPE_OFF){
            onTape[i] = FALSE; 
        }
        
        if (onTape[i]){
            result |= (1 << i); 
        }
    }
    
    return result;
}



uint8_t CheckTape(void) {
    
    
    ES_Event thisEvent;
    uint8_t returnVal = FALSE;
    uint8_t bits = Tape_GetBits();
    
    if (bits != lastBits) { // check for change from last time
        thisEvent.EventType = TAPE_RAW;
        thisEvent.EventParam = ((uint16_t) (bits ^ lastBits) << 8) | bits;
        returnVal = TRUE;
        lastBits = bits;
#ifndef EVENTCHECKER_TEST           // keep this as is for test harness
        PostTapeService(thisEvent);
#else
        SaveEvent(thisEvent);
#endif   
    }
    return (returnVal);
}


#ifdef EVENTCHECKER_TEST
#include <stdio.h>
static uint8_t(*EventList[])(void) = {EVENT_CHECK_LIST};

void PrintEvent(void);

void main(void) {
    BOARD_Init();
    /* user initialization code goes here */
    AD_Init();
    TapeEventChecker_Init();
    
    // Do not alter anything below this line
    int i;

    printf("\r\nEvent checking test harness for %s", __FILE__);

    while (1) {
        if (IsTransmitEmpty()) {
            for (i = 0; i< sizeof (EventList) >> 2; i++) {
                if (EventList[i]() == TRUE) {
                    PrintEvent();
                    break;
                }

            }
        }
    }
}

void PrintEvent(void) {
    printf("\r\nFunc: %s\tEvent: %s\tParam: 0x%X", eventName,
            EventNames[storedEvent.EventType], storedEvent.EventParam);
}
#endif