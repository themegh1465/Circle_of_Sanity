// Stepper header file for Circle of Sanity

#ifndef STEPPER_H
#define STEPPER_H

#include "stdint.h"

#define STEPPER_STEPS_PER_REV 200 // specific to each servo
#define STEPPER_CHAMBERS 7 // how many ping pong ball chambers


// sets up the pins
void Stepper_Init(void);

// takes in steps and advances stepper that many steps
// blocking (only for testing)
void Stepper_Step(int steps);

// advances the stepper to the next chamber
void Stepper_NextChamber(void);

// init half chamber move
void Stepper_InitHalfChamber(void);

// returns the amount of steps moved
int Stepper_GetSteps(void);

#endif 
