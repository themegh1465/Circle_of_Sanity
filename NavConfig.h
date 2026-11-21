/* 
 * File:   NavConfig.h
 * Author: meghp
 *
 * Created on November 17, 2026, 1:09 PM
 */

#ifndef NAVCONFIG_H
#define	NAVCONFIG_H

// different motor speeds for different types of turns
#define FOLLOW_SPEED 700
#define GENTLE_TURN 100
#define HARD_TURN 150

// time in ms to declare lost staet if all tape sensors go low
#define LOST_MS 2000

#define PIVOT_SPEED 700
#define T_REV_MS 2300 // time for the bot to make one full turn at set pivot speed, THIS NEEDS TO BE CALIBRATED REALYL WELL
#define deg_to_ms(deg) (((uint32_t)(deg) * T_REV_MS) / 360U) // convert a degree input into the time needed for the motors to be on for

#define CORNER_SPEED 600 // slow creep forward in cornertoedge state
#define CORNER_GUARD_MS 2000 // if FC off isnt seen within then lost_stop state
#define SETTLE_MS 50

#define TURN_MIN_DEG 30
#define TURN_EXTRA_DEG 10
#define TURN_SAFETY_DEG 120

#define APPROACH_SPEED 700 // looking for tape speed
#define APPROACH_GUARD_MS 20000 // longer than it should take to cross the field, if nothign seen within then, then loststop
#define ACQ_TURN_MAX_DEG 60 // most TurnToTape will pivot before giving up and creeping

#endif	/* NAVCONFIG_H */

