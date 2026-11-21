/*
 * File:   motor.c
 * Author: Megh Patel
 *
 */

#include <BOARD.h>
#include <pwm.h>
#include <IO_Ports.h>
#include "motor.h"

// pinout
#define LEFT_PWM_PIN PWM_PORTZ06
#define RIGHT_PWM_PIN PWM_PORTY12
#define DIR_PORT PORTZ
#define LEFT_IN_A PIN3
#define LEFT_IN_B PIN4
#define RIGHT_IN_A PIN5
#define RIGHT_IN_B PIN7

//flywheel pinout
#define FLY_LEFT_PWM_PIN PWM_PORTY10 // Y10 (ENA)
#define FLY_RIGHT_PWM_PIN PWM_PORTX11 // X11 (ENB)
#define FLY_DIR_PORT PORTX // dir port (JP7 has to stay open according to uno guide)
#define FLY_LEFT_IN_A PIN5 // X05
#define FLY_LEFT_IN_B PIN6 // X06
#define FLY_RIGHT_IN_A PIN8 // X08
#define FLY_RIGHT_IN_B PIN10 // X10

#define MOTOR_PWM_FREQ PWM_500HZ // if we chnage this, then we have to recalibrate the trim

// trim levels, keep separate values for forward and backwards
#define LEFT_TRIM_FWD 980
#define RIGHT_TRIM_FWD 1000
#define LEFT_TRIM_REV 980
#define RIGHT_TRIM_REV 1000

// high if the wheels spin backwards relative to the way they were plugged in
#define LEFT_INVERT 0
#define RIGHT_INVERT 0

// flywheel trim
#define FLY_LEFT_TRIM 1000
#define FLY_RIGHT_TRIM 1000

// motor dir invert
#define FLY_LEFT_INVERT 0
#define FLY_RIGHT_INVERT 0

// min signal needed to get motors to turn with full robot weight, not just floating
#define MIN_DUTY 0


#define SUCCESS 1
#define ERROR 0


#define ABS(x) ((x) < 0 ? -(x) : (x))

static int leftSpeed = 0;
static int rightSpeed = 0;
static int flywheelSpeed = 0;

static int ClampSpeed(int s)
{
    if (s > MOTOR_MAX_SPEED){
        return MOTOR_MAX_SPEED;
    }
    
    if (s < -MOTOR_MAX_SPEED){
        return -MOTOR_MAX_SPEED;
    }
    return s;
}

// convert the speed into a duty int based on trim
static unsigned int SpeedToDuty(int magnitude, int trim)
{
    long duty;

    if (magnitude == 0){
        return 0;
    }
    
    duty = MIN_DUTY + ((long) magnitude * (MOTOR_MAX_SPEED - MIN_DUTY)) / MOTOR_MAX_SPEED;
    duty = (duty * trim) / 1000;

    if (duty > MAX_PWM){
        duty = MAX_PWM;
    } else if (duty < 0){
        duty = 0;
    }
    return (unsigned int) duty;
}

// drive one side of the H-bridge
static void ApplyWheel(int speed, int invert, int trimFwd, int trimRev, unsigned short pwmPin, unsigned short inA, unsigned short inB)
{
    if (invert) {
        speed = -speed;
    }

    // if forward then A goes high and B goes low
    if (speed > 0) {
        IO_PortsSetPortBits(DIR_PORT, inA);
        IO_PortsClearPortBits(DIR_PORT, inB);
        PWM_SetDutyCycle(pwmPin, SpeedToDuty(speed, trimFwd));
    } else if (speed < 0) { // reverse
        IO_PortsClearPortBits(DIR_PORT, inA);
        IO_PortsSetPortBits(DIR_PORT, inB);
        PWM_SetDutyCycle(pwmPin, SpeedToDuty(-speed, trimRev));
    } else if (speed == 0){
        PWM_SetDutyCycle(pwmPin, 0);
    }
}

char Motor_Init(void)
{
    char ok = SUCCESS;
    if (PWM_Init() == ERROR) ok = ERROR;
    if (PWM_SetFrequency(MOTOR_PWM_FREQ) == ERROR) ok = ERROR;
    if (PWM_AddPins(LEFT_PWM_PIN | RIGHT_PWM_PIN) == ERROR) ok = ERROR;

    IO_PortsSetPortOutputs(DIR_PORT, LEFT_IN_A | LEFT_IN_B | RIGHT_IN_A | RIGHT_IN_B); // portz, z03, z04, z05, z07

    Motor_Brake(); // start with dead zero
    return ok; 
}

void Motor_SetLeft(int speed)
{
    leftSpeed = ClampSpeed(speed);
    ApplyWheel(leftSpeed, LEFT_INVERT, LEFT_TRIM_FWD, LEFT_TRIM_REV, LEFT_PWM_PIN, LEFT_IN_A, LEFT_IN_B);
}

void Motor_SetRight(int speed)
{
    rightSpeed = ClampSpeed(speed);
    ApplyWheel(rightSpeed, RIGHT_INVERT, RIGHT_TRIM_FWD, RIGHT_TRIM_REV, RIGHT_PWM_PIN, RIGHT_IN_A, RIGHT_IN_B);
}

void Motor_Drive(int left, int right)
{
    Motor_SetLeft(left);
    Motor_SetRight(right);
}

void Motor_Forward(int speed){ 
    speed = ABS(speed); 
    Motor_Drive(speed, speed); 
}
void Motor_Reverse(int speed){
    speed = ABS(speed); 
    Motor_Drive(-speed, -speed); 
}
void Motor_PivotCW(int speed){ 
    speed = ABS(speed); 
    Motor_Drive(speed, -speed); 
}
void Motor_PivotCCW(int speed){ 
    speed = ABS(speed); 
    Motor_Drive(-speed, speed); 
}

void Motor_Arc(int speed, int turn)
{
    Motor_Drive(speed + turn, speed - turn);
}

void Motor_Brake(void)
{
    // short oppositte ends of the h bridge for each motor, this will short both the motors to rail or ground for a hard stop
    // shorting to LOW on each dir pin so that the back emf is dumped to ground instead
    leftSpeed = 0;
    rightSpeed = 0;
    IO_PortsClearPortBits(DIR_PORT, LEFT_IN_A | LEFT_IN_B | RIGHT_IN_A | RIGHT_IN_B);
    PWM_SetDutyCycle(LEFT_PWM_PIN, MAX_PWM);
    PWM_SetDutyCycle(RIGHT_PWM_PIN, MAX_PWM);
}

void Motor_Coast(void)
{
    leftSpeed = 0;
    rightSpeed = 0;
    PWM_SetDutyCycle(LEFT_PWM_PIN, 0);
    PWM_SetDutyCycle(RIGHT_PWM_PIN, 0);
}

int Motor_GetLeft(void){ 
    return leftSpeed;   
}
int Motor_GetRight(void){ 
    return rightSpeed; 
}

void Flywheel_SetSpeed(int speed){

    if (speed < 0){
        speed = 0;
    }
    
    flywheelSpeed = ClampSpeed(speed);
    
    ApplyWheel(speed, FLY_LEFT_INVERT, FLY_LEFT_TRIM, FLY_LEFT_TRIM, FLY_LEFT_PWM_PIN, FLY_LEFT_IN_A, FLY_LEFT_IN_B);
    ApplyWheel(speed, FLY_RIGHT_INVERT, FLY_RIGHT_TRIM, FLY_RIGHT_TRIM, FLY_RIGHT_PWM_PIN, FLY_RIGHT_IN_A, FLY_RIGHT_IN_B);
}

void Flywheel_Stop(void)
{
    flywheelSpeed = 0;
    PWM_SetDutyCycle(FLY_LEFT_PWM_PIN, 0);
    PWM_SetDutyCycle(FLY_RIGHT_PWM_PIN, 0);
}

int Flywheel_GetSpeed(void){
    return flywheelSpeed;
}