#ifndef BANNEREVENTCHECKER_H
#define BANNEREVENTCHECKER_H

#include "ES_Configure.h"
#include "BOARD.h"

// init banner sensor
void Banner_Init(void);

// raw adc reading
uint16_t Banner_GetRaw(void);

// last debounced state from the state machine
uint8_t Banner_IsOn(void);

// event checker
uint8_t CheckBanner(void);

#endif