/* 
 * File:   BumperEventChecker.h
 * Author: meghp
 *
 * Created on November 20, 2026, 6:01 PM
 */

#ifndef BUMPEREVENTCHECKER_H
#define	BUMPEREVENTCHECKER_H

#include "ES_Configure.h"
#include "BOARD.h"

#define BUMP_FL 0x01 // Y11
#define BUMP_FR 0x02 // Z11
#define BUMP_RL 0x04 // Y08
#define BUMP_RR 0x08 // Y09

#define BUMP_FRONT (BUMP_FL | BUMP_FR)
#define BUMP_REAR (BUMP_RL | BUMP_RR)

// setup bumpers and pins
void Bumper_Init(void);

// get raw reading from the input, return bitmasked value
uint8_t Bumper_GetRaw(void);

// get last debounced sample, return bitmasked value
uint8_t Bumper_GetBits(void);

// bumper event checker
uint8_t CheckBumpers(void);


#endif	/* BUMPEREVENTCHECKER_H */

