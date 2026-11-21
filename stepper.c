#include <xc.h>
#include <BOARD.h>
#include <IO_Ports.h>
#include "stepper.h"

#define STEPPER_PORT PORTY
#define STEP_PIN PIN5 // Y05
#define DIR_PIN PIN3  // Y03

#define STEPPER_INVERT 0

#define STEP_PERIOD_US 4000 // time per full step (250 steps/s), raise this if the motor skips or buzzes
#define STEP_PULSE_US 10    // STEP high time, the A4988 only needs 1 us
#define TICKS_PER_US 40     // core timer runs at 80 MHz / 2

static int steps = 0;      // steps moved since init
static uint8_t chamber = 0; // which chamber the disc is on, 0 to 6

// busy wait on the core timer
static void WaitUs(uint32_t us)
{
    uint32_t start = _CP0_GET_COUNT();
    while ((uint32_t) (_CP0_GET_COUNT() - start) < us * TICKS_PER_US) { }
}

// step count from chamber 0 to chamber n, rounded to the nearest whole step
static int ChamberToSteps(uint8_t n)
{
    return ((int) n * STEPPER_STEPS_PER_REV + STEPPER_CHAMBERS / 2) / STEPPER_CHAMBERS;
}

void Stepper_Init(void)
{
    IO_PortsSetPortOutputs(STEPPER_PORT, STEP_PIN | DIR_PIN);
    IO_PortsClearPortBits(STEPPER_PORT, STEP_PIN | DIR_PIN);
    steps = 0;
    chamber = 0;
}

void Stepper_Step(int n)
{
    uint8_t forward = (n > 0) ? 1 : 0;
    int count = (n < 0) ? -n : n;
    int i;

    // set direction first, then give the driver a moment before the first pulse
    if (forward ^ STEPPER_INVERT) {
        IO_PortsSetPortBits(STEPPER_PORT, DIR_PIN);
    } else {
        IO_PortsClearPortBits(STEPPER_PORT, DIR_PIN);
    }
    WaitUs(5);

    for (i = 0; i < count; i++) {
        IO_PortsSetPortBits(STEPPER_PORT, STEP_PIN); // driver steps on the rising edge
        WaitUs(STEP_PULSE_US);
        IO_PortsClearPortBits(STEPPER_PORT, STEP_PIN);
        WaitUs(STEP_PERIOD_US - STEP_PULSE_US);
    }

    steps += n;
}

void Stepper_NextChamber(void)
{
    // 29, 28, 29, 28, 29, 28, 29 = 200 steps for a full lap of 7
    Stepper_Step(ChamberToSteps(chamber + 1) - ChamberToSteps(chamber));
    chamber = (chamber + 1) % STEPPER_CHAMBERS;
}

void Stepper_InitHalfChamber(void)
{
    // Init with 14 steps
    Stepper_Step(14);
}

int Stepper_GetSteps(void)
{
    return steps;
}