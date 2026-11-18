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
#include "TapeNavSubHSM.h"
#include "motor.h"
#include "TapeEventChecker.h"
#include "TapeService.h"
#include "NavConfig.h"

/*******************************************************************************
 * MODULE #DEFINES                                                             *
 ******************************************************************************/
typedef enum {
    InitPSubState,
    Follow,
            CornerToEdge,
            CornerSettle,
            CornerTurn,
            LostStop,
} TapeNavSubHSMState_t;

static const char *StateNames[] = {
	"InitPSubState",
	"Follow",
    "CornerToEdge",
    "CornerSettle",
    "CornerTurn",
    "LostStop",
};

// last seen inline or an edge
typedef enum {
    INLINE_SEEN,
            EDGE_SEEN,
} LastSeen_t;

/*******************************************************************************
 * PRIVATE FUNCTION PROTOTYPES                                                 *
 ******************************************************************************/
/* Prototypes for private functions for this machine. They should be functions
   relevant to the behavior of this state machine */

static void Steer(uint8_t bits); // truth table for follow


/*******************************************************************************
 * PRIVATE MODULE VARIABLES                                                            *
 ******************************************************************************/
/* You will need MyPriority and the state variable; you may need others as well.
 * The type of state variable should match that of enum in header file. */

static TapeNavSubHSMState_t CurrentState = InitPSubState; // <- change name to match ENUM
static LastSeen_t lastSeen = INLINE_SEEN;
static uint8_t MyPriority;
static uint32_t turnStart = 0; // ms for when the pivot started

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
uint8_t InitTapeNavSubHSM(void)
{
    ES_Event returnEvent;
    
    CurrentState = InitPSubState;
    returnEvent = RunTapeNavSubHSM(INIT_EVENT);
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
ES_Event RunTapeNavSubHSM(ES_Event ThisEvent)
{
    uint8_t makeTransition = FALSE; // use to flag transition
    TapeNavSubHSMState_t nextState; // <- change type to correct enum

    uint8_t current = ThisEvent.EventParam & 0xFF; // low byte for tape sensors that are on now
    uint8_t changed = ThisEvent.EventParam >> 8; // high byte for tape sensors that JUST changed
    uint8_t rose = current & changed; // which tape sensors just went onto the tape
    uint8_t previous = changed ^ current; // what tape sensors were on before
    
    ES_Tattle(); // trace call stack

    
    
    
    // state machine //
    
    
    switch (CurrentState) {
    case InitPSubState: // If current state is initial Psedudo State
        if (ThisEvent.EventType == ES_INIT)// only respond to ES_Init
        {
            // now put the machine into the actual initial state
            nextState = Follow;
            makeTransition = TRUE;
            ThisEvent.EventType = ES_NO_EVENT;
        }
        break;
        
    // FOLLOW STATE

    case Follow:
        switch (ThisEvent.EventType) {
            case ES_ENTRY:
                lastSeen = INLINE_SEEN;
                Steer(TapeService_GetBits());
                ThisEvent.EventType = ES_NO_EVENT;
                break;
        
            case TAPE_CHANGED:
                // check if we saw edge or inline before we decide next move in truth table
                if (current & TAPE_E){
                    lastSeen = EDGE_SEEN;
                } else if (current & (TAPE_RL | TAPE_FL)){
                    lastSeen = INLINE_SEEN;
                }
                
                // ideally either FC or FR go off:
                // only FC: then we have seen corner move to the cornertoedge state
                // only FR: FC is already past the tape go to corner settle state
                if (rose & (TAPE_FC | TAPE_FR)){
                    nextState = CornerToEdge;
                    makeTransition = TRUE;
                } else {
                    // just continue if its not the sensors we care about
                    Steer(current);
                    
                    // check if we havent lost the robot
                    // we lose the robot if RL, FL, E all go dark if we saw them previously
                    // start the lost timer, and if we still lost after timeout, we just stop the robot
                    
                    if(!(current & (TAPE_RL | TAPE_FL | TAPE_E))){
                        if(previous & (TAPE_RL | TAPE_FL | TAPE_E)){
                            ES_Timer_InitTimer(NAV_LOST_TIMER, LOST_MS);
                        }
                    } else {
                        // if either RL, FL, or E is seen again then we arent lost anymore
                        ES_Timer_StopTimer(NAV_LOST_TIMER);
                    }
                    
                }
                ThisEvent.EventType = ES_NO_EVENT;
                break;
                
            case ES_TIMEOUT:
                if (ThisEvent.EventParam == NAV_LOST_TIMER && !(TapeService_GetBits() & (TAPE_RL | TAPE_FL | TAPE_E))){
                    nextState = LostStop;
                    makeTransition = TRUE;
                    ThisEvent.EventType = ES_NO_EVENT;
                }
                break;
            
            case ES_EXIT:
                ES_Timer_StopTimer(NAV_LOST_TIMER);
                break;
            
            default: // all unhandled events pass the event back up to the next level
                break;
        }
        break;
    
        
    // CORNERTOEDGE STATE 
        
    case CornerToEdge: // creep forward until FC goes low
        
        switch(ThisEvent.EventType){
            
            case ES_ENTRY:
                Motor_Forward(CORNER_SPEED);
                ES_Timer_InitTimer(NAV_GUARD_TIMER, CORNER_GUARD_MS);
                ThisEvent.EventType = ES_NO_EVENT;
                break;
            
            case TAPE_CHANGED:
                if (changed & ~current & TAPE_FC) { // if FC changed and was not already on
                    nextState = CornerSettle;
                    makeTransition = TRUE;
                }
                ThisEvent.EventType = ES_NO_EVENT;
                break;
                
            case ES_TIMEOUT:
                if (ThisEvent.EventParam == NAV_GUARD_TIMER){
                    nextState = LostStop;
                    makeTransition = TRUE;
                    ThisEvent.EventType = ES_NO_EVENT;
                }
                break;
                
            case ES_EXIT:
                ES_Timer_StopTimer(NAV_GUARD_TIMER);
                break;
            
            default:
                break;
        }
        break;
        
        
    // CORNERSETTLE STATE
        
    case CornerSettle: // brake to settle the robot momentum and then make the right turn 
        switch(ThisEvent.EventType){
            case ES_ENTRY:
                Motor_Brake();
                ES_Timer_InitTimer(NAV_MOTION_TIMER, SETTLE_MS);
                ThisEvent.EventType = ES_NO_EVENT;
                break;
                
            case ES_TIMEOUT:
                if (ThisEvent.EventParam == NAV_MOTION_TIMER){
                    nextState = CornerTurn;
                    makeTransition = TRUE;
                    ThisEvent.EventType = ES_NO_EVENT;
                }
                break;
                
            case TAPE_CHANGED: // dont care about any possible flicker or extra states
                ThisEvent.EventType = ES_NO_EVENT;
                break;
            default:
                break;
        }
        break;
        
        
    // CORNERTURN STATE
        
    case CornerTurn:
        switch(ThisEvent.EventType){
            case ES_ENTRY:
                turnStart = ES_Timer_GetTime();
                Motor_PivotCW(PIVOT_SPEED); //90 degree turn clockwise
                ES_Timer_InitTimer(NAV_GUARD_TIMER, deg_to_ms(TURN_SAFETY_DEG));
                ThisEvent.EventType = ES_NO_EVENT;
                break;
            case ES_TIMEOUT:
                if(ThisEvent.EventParam == NAV_MOTION_TIMER){ // stop the 90 degree turn and then go straight back to follow state
                    nextState = Follow;
                    makeTransition = TRUE;
                    ThisEvent.EventType = ES_NO_EVENT;
                }
                break;
            case TAPE_CHANGED:
                if ((changed & ~current & TAPE_FC) && (ES_Timer_GetTime() - turnStart) > deg_to_ms(TURN_MIN_DEG)){
                    ES_Timer_StopTimer(NAV_GUARD_TIMER);
                    if (TURN_EXTRA_DEG == 0){
                        nextState = Follow;
                        makeTransition = TRUE;
                    } else {
                        ES_Timer_InitTimer(NAV_MOTION_TIMER, deg_to_ms(TURN_EXTRA_DEG));
                    }
                }
                ThisEvent.EventType = ES_NO_EVENT;
                break;
            case ES_TIMEOUT:
                if (ThisEvent.EventParam == NAV_MOTION_TIMER){ //extra spin
                    nextState = Follow;
                    makeTransition = TRUE;
                    ThisEvent.EventType = ES_NO_EVENT;
                } else if (ThisEvent.EventParam == NAV_GUARD_TIMER){  //FC never fell
                    nextState = LostStop;
                    makeTransition = TRUE;
                    ThisEvent.EventType = ES_NO_EVENT;
                }
                break;
            case ES_EXIT:
                Motor_Brake();
                ES_Timer_StopTimer(NAV_MOTION_TIMER);
                ES_Timer_StopTimer(NAV_GUARD_TIMER);
                break;
            default:
                break;
        }
        break;
        
        
    // LOSTSTOP (temp state for testing)
        
    case LostStop:
        if (ThisEvent.EventType == ES_ENTRY) {
            Motor_Brake();
            ThisEvent.EventType = ES_NO_EVENT;
        } else if (ThisEvent.EventType == TAPE_CHANGED) {
            ThisEvent.EventType = ES_NO_EVENT;
        }
        break;
      
        
    default: // all unhandled states fall into here
        break;
    } // end switch on Current State

    if (makeTransition == TRUE) { // making a state transition, send EXIT and ENTRY
        // recursively call the current state with an exit event
        RunTapeNavSubHSM(EXIT_EVENT); // <- rename to your own Run function
        CurrentState = nextState;
        RunTapeNavSubHSM(ENTRY_EVENT); // <- rename to your own Run function
    }

    ES_Tail(); // trace call stack end
    return ThisEvent;
}


/*******************************************************************************
 * PRIVATE FUNCTIONS                                                           *
 ******************************************************************************/

static void Steer(uint8_t bits){
    // E is bit 2, FL bit 3, RL bit 4 (set in eventchecker)
    uint8_t e = (bits & TAPE_E) ? 1 : 0;
    uint8_t fl = (bits & TAPE_FL) ? 1 : 0;
    uint8_t rl = (bits & TAPE_RL) ? 1 : 0;
    
    int turn;
    
    int gentle_left = -GENTLE_TURN;
    int gentle_right = GENTLE_TURN;
    int hard_left = -HARD_TURN;
    int hard_right =  HARD_TURN;
    
    switch ((fl << 2) | (rl << 1) | e){
        case 0b110:
            turn = 0;
            break;
        case 0b111:
            turn = gentle_left;
            break;
        case 0b010:
            turn = gentle_right;
            break;
        case 0b100:
            turn = gentle_left;
            break;
        case 0b011:
            turn = hard_left;
            break;
        case 0b101:
            turn = gentle_right;
            break;
        case 0b001:
            turn = hard_left;
            break;
        default:
            turn = (lastSeen == EDGE_SEEN) ? hard_left : hard_right;
            break;
    }
    
    Motor_Arc(FOLLOW_SPEED, turn);
}
