#ifndef BEACONDETECTOR_H
#define	BEACONDETECTOR_H

#include <stdint.h>

#define BEACON_NOT_FOUND 0xFFFF // param if nothing found

// init with AD library
void Beacon_Init(void);

// latest RAW reading (0-1023)
uint16_t Beacon_ReadLevel(void);

// clear the bins and rescan the field
void Beacon_StartScan(void);

// TRUE while a scan is recording
uint8_t Beacon_IsScanning(void);

// results of the last scan
uint8_t Beacon_Found(void); // TRUE if the peak stood out of the noise floor
int16_t Beacon_GetAngle(void); // degrees CW from the scan start heading, 0 to 359
int16_t Beacon_GetTurnDeg(void); // shortest turn back to the beacon from the start heading, + = CW, - = CCW
uint16_t Beacon_GetPeakLevel(void); // smoothed peak, ADC counts
uint16_t Beacon_GetFloorLevel(void); // noise floor, ADC counts
uint16_t Beacon_GetWidthDeg(void); // how wide the peak is at half height, degrees

// event checker
uint8_t CheckBeacon(void);

#endif