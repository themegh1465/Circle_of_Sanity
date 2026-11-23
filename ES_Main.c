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

#define TICKS_PER_MS 40000UL


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
    

    printf("Flywheel test\r\n");
    Wait(2000);
    printf("\nRamping up now");
    Flywheel_SetSpeed(500);
    Wait(30000);
    printf("\nStopping now, letting wheels slow down for 10 sec");
    Flywheel_Stop();
    Wait(10000);
    printf("\ntest done");
    
    /*
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
    */
};

/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/
