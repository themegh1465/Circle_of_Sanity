#ifndef AMMODISC_H
#define	AMMODISC_H

#include <stdint.h>

#define AMMO_STEPS_PER_REV 200
#define AMMO_CHAMBERS 7
#define AMMO_STEP_RATE 250 // steps per second, tune this

// starts the stepper library
void Ammo_Init(void);

// starts turning to the next chamber and returns right away. 200/7 isnt whole so it
// alternates 29 and 28 steps, 7 chambers = exactly 200
uint8_t Ammo_NextChamber(void);

// which chamber is lined up, 0 to 6 (0 = where the disc was at Ammo_Init)
uint8_t Ammo_GetChamber(void);

// event checker
uint8_t CheckAmmoDisc(void);

#endif