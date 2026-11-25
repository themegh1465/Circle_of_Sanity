/*
 * File: TemplateSubHSM.c
 * Author: J. Edward Carryer
 * Modified: Gabriel H Elkaim
 *
 * Template file to set up a Heirarchical State Machine to work with the Events and
 * Services Framework (ES_Framework) on the Uno32 for the CMPE-118/L class. Note that
 * this file will need to be modified to fit your exact needs, and most of the names
 * will have to be changed to match your code.
 *
 * There is for a substate machine. Make sure it has a unique name
 *
 * This is provided as an example and a good place to start.
 *
 * History
 * When           Who     What/Why
 * -------------- ---     --------
 * 09/13/13 15:17 ghe      added tattletail functionality and recursive calls
 * 01/15/12 11:12 jec      revisions for Gen2 framework
 * 11/07/11 11:26 jec      made the queue static
 * 10/30/11 17:59 jec      fixed references to CurrentEvent in RunTemplateSM()
 * 10/23/11 18:20 jec      began conversion from SMTemplate.c (02/20/07 rev)
 */


/*******************************************************************************
 * MODULE #INCLUDE                                                             *
 ******************************************************************************/

#include "ES_Configure.h"
#include "ES_Framework.h"
#include "BOARD.h"
#include "TopHSM.h"
#include "OpponentSubHSM.h"
#include "motor.h"
#include "AmmoDisc.h"
#include "BeaconDetector.h"
#include "NavConfig.h"

/*******************************************************************************
 * MODULE #DEFINES                                                             *
 ******************************************************************************/
typedef enum {
    InitPSubState,
    Scan,
            ScanSettle,
            Aim,
            WaitSpinUp,
            Fire,
            Reload,
} OpponentSubHSMState_t;

static const char *StateNames[] = {
    "InitPSubState",
    "Scan",
    "ScanSettle",
    "Aim",
    "WaitSpinUp",
    "Fire",
    "Reload",
};



/*******************************************************************************
 * PRIVATE FUNCTION PROTOTYPES                                                 *
 ******************************************************************************/
/* Prototypes for private functions for this machine. They should be functions
   relevant to the behavior of this state machine */

/*******************************************************************************
 * PRIVATE MODULE VARIABLES                                                            *
 ******************************************************************************/
/* You will need MyPriority and the state variable; you may need others as well.
 * The type of state variable should match that of enum in header file. */

static OpponentSubHSMState_t CurrentState = InitPSubState; // <- change name to match ENUM
static uint8_t MyPriority;
static uint8_t scansDone = 0; // how many 360 scna we did
static uint8_t shotsFired = 0;
static uint8_t flywheelReady = FALSE; //set to true once spinup timeout occurs

/*******************************************************************************
 * PUBLIC FUNCTIONS                                                            *
 ******************************************************************************/

/**
 * @Function InitTemplateSubHSM(uint8_t Priority)
 * @param Priority - internal variable to track which event queue to use
 * @return TRUE or FALSE
 * @brief This will get called by the framework at the beginning of the code
 *        execution. It will post an ES_INIT event to the appropriate event
 *        queue, which will be handled inside RunTemplateFSM function. Remember
 *        to rename this to something appropriate.
 *        Returns TRUE if successful, FALSE otherwise
 * @author J. Edward Carryer, 2011.10.23 19:25 */
uint8_t InitOpponentSubHSM(void)
{
    ES_Event returnEvent;

    CurrentState = InitPSubState;
    returnEvent = RunOpponentSubHSM(INIT_EVENT);
    if (returnEvent.EventType == ES_NO_EVENT) {
        return TRUE;
    }
    return FALSE;
}

/**
 * @Function RunTemplateSubHSM(ES_Event ThisEvent)
 * @param ThisEvent - the event (type and param) to be responded.
 * @return Event - return event (type and param), in general should be ES_NO_EVENT
 * @brief This function is where you implement the whole of the heirarchical state
 *        machine, as this is called any time a new event is passed to the event
 *        queue. This function will be called recursively to implement the correct
 *        order for a state transition to be: exit current state -> enter next state
 *        using the ES_EXIT and ES_ENTRY events.
 * @note Remember to rename to something appropriate.
 *       The lower level state machines are run first, to see if the event is dealt
 *       with there rather than at the current level. ES_EXIT and ES_ENTRY events are
 *       not consumed as these need to pass pack to the higher level state machine.
 * @author J. Edward Carryer, 2011.10.23 19:25
 * @author Gabriel H Elkaim, 2011.10.23 19:25 */
ES_Event RunOpponentSubHSM(ES_Event ThisEvent)
{
    uint8_t makeTransition = FALSE; // use to flag transition
    OpponentSubHSMState_t nextState; // <- change type to correct enum

    int16_t turn;
    uint32_t turnMs;
    
    ES_Tattle(); // trace call stack

    switch (CurrentState) {
    case InitPSubState: // If current state is initial Psedudo State
        if (ThisEvent.EventType == ES_INIT)// only respond to ES_Init
        {
            // this is where you would put any actions associated with the
            // transition from the initial pseudo-state into the actual
            // initial state
            
            scansDone = 0;
            shotsFired = 0;
            flywheelReady = FALSE;

            // now put the machine into the actual initial state
            nextState = Scan;
            makeTransition = TRUE;
            ThisEvent.EventType = ES_NO_EVENT;
        }
        break;

        
    // SCAN STATE
        
    case Scan:
        switch(ThisEvent.EventType){
            case ES_ENTRY:
                Motor_PivotCW(PIVOT_SPEED); // BeaconDetector assumes a CW spin at PIVOT_SPEED
                Beacon_StartScan();
                ThisEvent.EventType = ES_NO_EVENT;
                break;
            case BEACON_SCAN_DONE:
                Motor_Brake();
                Beacon_PrintScan(); // debug print
                scansDone++; // update how many scans we have done
                if (ThisEvent.EventParam != BEACON_NOT_FOUND) { // bearing was found
                    Flywheel_SetSpeed(FLYWHEEL_SPEED);
                    flywheelReady = FALSE;
                    ES_Timer_InitTimer(SHOOT_TIMER, SPINUP_MS);
                    nextState = ScanSettle;
                    makeTransition = TRUE;
                    ThisEvent.EventType = ES_NO_EVENT;
                } else if (scansDone < SCAN_TRIES) {
                    nextState = Scan; // try one more spin
                    makeTransition = TRUE;
                    ThisEvent.EventType = ES_NO_EVENT;
                } else {
                    ThisEvent.EventType = OPPONENT_DONE; // give up, TopHSM moves on to Navigate
                }
                break;
            default:
                break;
        }
        break;
        
        
    // SCAN SETTLE STATE
        
    case ScanSettle:
        switch (ThisEvent.EventType) {
            case ES_ENTRY:
                Motor_Brake();
                ES_Timer_InitTimer(NAV_MOTION_TIMER, SETTLE_MS);
                ThisEvent.EventType = ES_NO_EVENT;
                break;

            case ES_TIMEOUT:
                if (ThisEvent.EventParam == NAV_MOTION_TIMER) {
                    nextState = Aim;
                    makeTransition = TRUE;
                    ThisEvent.EventType = ES_NO_EVENT;
                } else if (ThisEvent.EventParam == SHOOT_TIMER) {
                    flywheelReady = TRUE;
                    ThisEvent.EventType = ES_NO_EVENT;
                }
                break;

            default:
                break;
        }
        break;
        
    
    // AIM STATE
        
    case Aim: // turn to the beacon with the flywheels spinning up
        switch (ThisEvent.EventType) {
            case ES_ENTRY:
                turn = Beacon_GetTurnDeg(); // if pos then CW and neg then CCW
                if (turn >= 0) {
                    Motor_PivotCW(PIVOT_SPEED);
                    turnMs = deg_to_ms(turn);
                } else {
                    Motor_PivotCCW(PIVOT_SPEED);
                    turnMs = deg_to_ms(-turn);
                }
                if (turnMs == 0) {
                    turnMs = 1; // edge case
                }
                ES_Timer_InitTimer(NAV_MOTION_TIMER, turnMs);
                ThisEvent.EventType = ES_NO_EVENT;
                break;

            case ES_TIMEOUT:
                if (ThisEvent.EventParam == NAV_MOTION_TIMER) {
                    nextState = flywheelReady ? Fire : WaitSpinUp; // go to wait spinup state if flywheels arent ready yet
                    makeTransition = TRUE;
                    ThisEvent.EventType = ES_NO_EVENT;
                } else if (ThisEvent.EventParam == SHOOT_TIMER) {
                    flywheelReady = TRUE;
                    ThisEvent.EventType = ES_NO_EVENT;
                }
                break;

            case ES_EXIT:
                Motor_Brake();
                ES_Timer_StopTimer(NAV_MOTION_TIMER);
                break;

            default:
                break;
        }
        break;
        
        
    // WAITSPINUP STATE

    case WaitSpinUp: // barrel lined up, waiting for flywheel spinup timer
        if (ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == SHOOT_TIMER) {
            flywheelReady = TRUE;
            nextState = Fire;
            makeTransition = TRUE;
            ThisEvent.EventType = ES_NO_EVENT;
        }
        break;
        
    
    // FIRE STATE

    case Fire: // bombs away, rotate stepper once
        switch (ThisEvent.EventType) {
            case ES_ENTRY:
                Ammo_NextChamber();
                ThisEvent.EventType = ES_NO_EVENT;
                break;

            case STEPPER_DONE:
                shotsFired++;
                nextState = Reload;
                makeTransition = TRUE;
                ThisEvent.EventType = ES_NO_EVENT;
                break;

            default:
                break;
        }
        break;
        
        
    // RELOAD STATE

    case Reload: // based on preconfig timer let the flywheels get back to max speed
        switch (ThisEvent.EventType) {
            case ES_ENTRY:
                ES_Timer_InitTimer(SHOOT_TIMER, RELOAD_MS);
                ThisEvent.EventType = ES_NO_EVENT;
                break;

            case ES_TIMEOUT:
                if (ThisEvent.EventParam == SHOOT_TIMER) {
                    if (shotsFired < OPPONENT_SHOTS) {
                        nextState = Fire;
                        makeTransition = TRUE;
                        ThisEvent.EventType = ES_NO_EVENT;
                    } else {
                        Flywheel_Stop();
                        ThisEvent.EventType = OPPONENT_DONE; // move TOP hsm to nav state once done
                    }
                }
                break;

            default:
                break;
        }
        break;
        
        
    default: // all unhandled states fall into here
        break;
    } // end switch on Current State

    if (makeTransition == TRUE) { // making a state transition, send EXIT and ENTRY
        // recursively call the current state with an exit event
        RunOpponentSubHSM(EXIT_EVENT); // <- rename to your own Run function
        CurrentState = nextState;
        RunOpponentSubHSM(ENTRY_EVENT); // <- rename to your own Run function
    }

    ES_Tail(); // trace call stack end
    return ThisEvent;
}


/*******************************************************************************
 * PRIVATE FUNCTIONS                                                           *
 ******************************************************************************/

