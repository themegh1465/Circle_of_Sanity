#include <BOARD.h>
#include <xc.h>
#include <stdio.h>
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "EventCheckers.h"
#include "motor.h"
#include "NavConfig.h"
#include "AmmoDisc.h"
#include <AD.h>


#define TICKS_PER_MS 40000UL // core timer runs at 40 MHz

// busy wait, only for bench tests outside the ES framework
static void Wait(unsigned long ms)
{
    unsigned long start = _CP0_GET_COUNT();
    while ((unsigned long) (_CP0_GET_COUNT() - start) < ms * TICKS_PER_MS) { }
}

void main(void)
{
    ES_Return_t ErrorType;

    BOARD_Init();

    printf("Starting ES Framework Template\r\n");
    printf("using the 2nd Generation Events & Services Framework\r\n");


    // Your hardware initialization function calls go here
    AD_Init();
    Motor_Init();
    TapeEventChecker_Init();
    Ammo_Init();
    Bumper_Init();
    Beacon_Init();
    Stepper_Init();
    Banner_Init();
   
    /*
    Wait(2000);
    Motor_PivotCW(PIVOT_SPEED);
    Wait(T_REV_MS);
    Motor_Brake();
    */
    
    
    Wait(2000);
    // now initialize the Events and Services Framework and start it running
    ErrorType = ES_Initialize();
    if (ErrorType == Success) {
        ErrorType = ES_Run();

    }
    //if we got to here, there was an error
    switch (ErrorType) {
    case FailedPointer:
        printf("Failed on NULL pointer");
        break;
    case FailedInit:
        printf("Failed Initialization");
        break;
    default:
        printf("Other Failure: %d", ErrorType);
        break;
    }
    for (;;)
        ;
    
};

/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/
