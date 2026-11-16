/* 
 * File:   TapeEventChecker.h
 * Author: Gabriel Hugh Elkaim + Megh Patel
 *
 * Created on September 27, 2013, 8:37 AM
 * Modified on September 12, 2016, 7:59 PM
 */

#ifndef TAPEEVENTCHECKER_H
#define	TAPEEVENTCHECKER_H

/*******************************************************************************
 * PUBLIC #INCLUDES                                                            *
 ******************************************************************************/

#include "ES_Configure.h"   // defines ES_Event, INIT_EVENT, ENTRY_EVENT, and EXIT_EVENT
#include "BOARD.h"

/*******************************************************************************
 * PUBLIC #DEFINES                                                             *
 ******************************************************************************/

// bitmask the tape sensors 
#define TAPE_FC 0x01 // 0000 0001
#define TAPE_E 0x02 // 0000 0010
#define TAPE_FL 0x04 // 0000 0100
#define TAPE_RL 0x08 // 0000 1000
#define TAPE_FR 0x10 // 0001 0000
#define TAPE_RR 0x20 // 0010 0000



/*******************************************************************************
 * PUBLIC TYPEDEFS                                                             *
 ******************************************************************************/


/*******************************************************************************
 * PUBLIC FUNCTION PROTOTYPES                                                  *
 ******************************************************************************/

// setup pins, called at startup
void TapeEventChecker_Init(void);

// convert the raw input back into the bitmask with hysteresis
uint8_t Tape_GetBits(void);

// the actual event checker
uint8_t CheckTape(void);



#endif

