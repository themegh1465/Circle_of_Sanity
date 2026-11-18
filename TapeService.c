/*******************************************************************************
 * MODULE #INCLUDE                                                             *
 ******************************************************************************/

#include "BOARD.h"
#include "AD.h"
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "TapeService.h"
#include "TopHSM.h"
#include <stdio.h>

/*******************************************************************************
 * MODULE #DEFINES                                                             *
 ******************************************************************************/

#define BATTERY_DISCONNECT_THRESHOLD 175

#define TAPE_DEBOUNCE_MS 5


/*******************************************************************************
 * PRIVATE FUNCTION PROTOTYPES                                                 *
 ******************************************************************************/
/* Prototypes for private functions for this machine. They should be functions
   relevant to the behavior of this state machine */

/*******************************************************************************
 * PRIVATE MODULE VARIABLES                                                    *
 ******************************************************************************/
/* You will need MyPriority and maybe a state variable; you may need others
 * as well. */

static uint8_t MyPriority;

static uint8_t pendingBits = 0; // latest param from event checker
static uint8_t stableBits = 0; // latest debounced and posted param

/*******************************************************************************
 * PUBLIC FUNCTIONS                                                            *
 ******************************************************************************/


uint8_t InitTapeService(uint8_t Priority)
{
    ES_Event ThisEvent;

    MyPriority = Priority;
    pendingBits = 0;
    stableBits = 0;

    // in here you write your initialization code
    // this includes all hardware and software initialization
    // that needs to occur.

    // post the initial transition event
    ThisEvent.EventType = ES_INIT;
    if (ES_PostToService(MyPriority, ThisEvent) == TRUE) {
        return TRUE;
    } else {
        return FALSE;
    }
}

uint8_t PostTapeService(ES_Event ThisEvent)
{
    return ES_PostToService(MyPriority, ThisEvent);
}

uint8_t TapeService_GetBits(void){
    return stableBits;
}

ES_Event RunTapeService(ES_Event ThisEvent)
{
    ES_Event PostEvent;
    ES_Event ReturnEvent;
    ReturnEvent.EventType = ES_NO_EVENT; // assume no errors

    /********************************************
     in here you write your service code
     *******************************************/
    switch (ThisEvent.EventType) {
    case ES_INIT:
        // No hardware initialization or single time setups, those
        // go in the init function above.
        //
        // This section is used to reset service for some reason
        break;
        
    case TAPE_RAW:
        pendingBits = ThisEvent.EventParam & 0x00FF; // get the lower 8 bits
        ES_Timer_InitTimer(TAPE_DEBOUNCE_TIMER, TAPE_DEBOUNCE_MS);
        break;

    case ES_TIMEOUT:
        if (ThisEvent.EventParam != TAPE_DEBOUNCE_TIMER){
            break;
        }
        if (pendingBits != stableBits){
            PostEvent.EventType = TAPE_CHANGED;
            PostEvent.EventParam = ((uint16_t) (pendingBits ^ stableBits) << 8) | pendingBits;
            stableBits = pendingBits;
            
#ifndef SIMPLESERVICE_TEST           // keep this as is for test harness
            PostTopHSM(PostEvent);
#else
            PostTapeService(PostEvent);
#endif   
        }
        break;
        
    case ES_TIMERACTIVE:
    case ES_TIMERSTOPPED:
        break;
        
    default:
#ifdef SIMPLESERVICE_TEST     // keep this as is for test harness      
    
        printf("\r\nEvent: %s\tParam: 0x%X",
                EventNames[ThisEvent.EventType], ThisEvent.EventParam);
        break;
#endif
    }

    return ReturnEvent;
}

/*******************************************************************************
 * PRIVATE FUNCTIONs                                                           *
 ******************************************************************************/

