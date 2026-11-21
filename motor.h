/* 
 * File:   motor.h
 * Author: meghp
 *
 * Created on November 13, 2026, 6:35 PM
 */

#ifndef MOTOR_H
#define	MOTOR_H

#include <stdint.h>

#define MOTOR_MAX_SPEED 1000

// Sets up PWM and the direction pins and leaves both motors braked
char Motor_Init(void);

// Sets the left wheel speed, -1000 to 1000
void Motor_SetLeft(int speed);

// Sets the right wheel speed, -1000 to 1000
void Motor_SetRight(int speed);

// Sets both wheel speeds at once, -1000 to 1000 each
void Motor_Drive(int left, int right);

// Drives straight forward at speed 0 to 1000
void Motor_Forward(int speed);

// Drives straight backward at speed 0 to 1000
void Motor_Reverse(int speed);

// Spins clockwise on the robot's center (left wheel forward, right wheel back) at speed 0 to 1000
void Motor_PivotCW(int speed);

// Spins counterclockwise on the robot's center (left wheel back, right wheel forward) at speed 0 to 1000
void Motor_PivotCCW(int speed);

// Drives while steerin, left wheel gets speed + turn, right wheel gets speed - turn
void Motor_Arc(int speed, int turn);

// Hard brake, shorts both motors through the H-bridge, used between timed turns
void Motor_Brake(void);

// turns both enables off and lets the motors spin down on their own
void Motor_Coast(void);

// Returns the last speed commanded to the left wheel
int Motor_GetLeft(void);

// Returns the last speed commanded to the right wheel
int Motor_GetRight(void);

// flywheels

// opposite directions and speed 0-1000 scaled by trim
void Flywheel_SetSpeed(int speed);

// just cuts power, no hard brake
void Flywheel_Stop(void);

// returns last speed sent to flywheels
int Flywheel_GetSpeed(void);

#endif // MOTOR_H

