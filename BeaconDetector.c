/*
How the scan works:
1. Sampling: every ES loop while scanning, read W04 and drop the reading into the
angle bin for how far into the spin we are. The pivot speed is constant, so
time since the scan started maps straight to angle: bin = t * NUM_BINS / T_REV_MS.
2. Storing: each bin keeps a running sum and a count, so it ends up as the average
of every reading taken in that 5 degree slice (about 30+ readings per bin).
3. Filtering: a symmetric [1 2 1] smooth around the circle. Symmetric means it
doesnt shift the peak sideways. This is sitll a placeholder filter, if we dont need it then dont include it
4. Peak finding: noise floor = average of the quietest quarter of bins, peak = the
highest bin. half height =  floor + (peak - floor) / 2. The beacon counts as found if the peak clears the floor by BEACON_MIN_RISE. Tune this value.
5. Angle: take every bin around the peak that is above half the peak's height and
find their weighted center (center of mass). That lands between bins, so it is
finer than the 5 degree bins and doesnt get fooled by a flat topped peak.
*/

#include "ES_Configure.h"
#include "ES_Framework.h"
#include "BOARD.h"
#include "AD.h"
#include "TopHSM.h"
#include "NavConfig.h"
#include "BeaconDetector.h"

/*******************************************************************************
 * MODULE #DEFINES                                                             *
 ******************************************************************************/

#define BEACON_PIN AD_PORTW4

#define NUM_BINS 72 // 5 degrees per bin
#define BEACON_MIN_RISE 60 // minimum counts above floor seen to classify as a peak (ideally we would want to gain signal if this is not met) ~0.2V
#define BEACON_ANGLE_OFFSET_DEG 0 // calibrate any extra offset from filter delay or just overall turning

/*******************************************************************************
 * PRIVATE MODULE VARIABLES                                                    *
 ******************************************************************************/

static uint32_t binSum[NUM_BINS]; // running sum of readings in each bin
static uint16_t binCount[NUM_BINS]; // how many readings went into each bin
static uint16_t profile[NUM_BINS]; // averaged and smoothed level per bin

static uint8_t scanning = FALSE;
static uint32_t scanStart = 0; // ms when the scan started

static uint8_t found = FALSE;
static int16_t angleDeg = 0;
static uint16_t peakLevel = 0;
static uint16_t floorLevel = 0;
static uint16_t widthDeg = 0;

/*******************************************************************************
 * PRIVATE FUNCTIONS                                                           *
 ******************************************************************************/

// wraps a bin index around the circle
static int Wrap(int i)
{
    while (i < 0) i += NUM_BINS;
    while (i >= NUM_BINS) i -= NUM_BINS;
    return i;
}

// passes them straight through for now
static uint16_t FilterSample(uint16_t raw)
{
    return raw;
}

// turns the bin sums into a smoothed profile, then finds the floor, peak and angle
static void Analyze(void)
{
    uint16_t avg[NUM_BINS];
    uint16_t sorted[NUM_BINS];
    int i, j, pk;
    uint32_t sum;
    uint16_t half;
    int32_t weightSum, weightedPos;
    int left, right, offset;

    // average each bin, a bin with no readings borrows from its neighbors later
    for (i = 0; i < NUM_BINS; i++) {
        avg[i] = binCount[i] ? (uint16_t) (binSum[i] / binCount[i]) : 0xFFFF;
    }
    for (i = 0; i < NUM_BINS; i++) {
        if (avg[i] == 0xFFFF) {
            uint16_t a = avg[Wrap(i - 1)], b = avg[Wrap(i + 1)];
            avg[i] = (a == 0xFFFF) ? ((b == 0xFFFF) ? 0 : b) : ((b == 0xFFFF) ? a : (a + b) / 2);
        }
    }

    // symmetric [1 2 1] smooth
    for (i = 0; i < NUM_BINS; i++) {
        profile[i] = (avg[Wrap(i - 1)] + 2 * avg[i] + avg[Wrap(i + 1)]) / 4;
    }

    // noise floor = average of the quietest quarter of the bins
    for (i = 0; i < NUM_BINS; i++) {
        sorted[i] = profile[i];
    }
    for (i = 1; i < NUM_BINS; i++) {
        uint16_t v = sorted[i];
        for (j = i - 1; j >= 0 && sorted[j] > v; j--) {
            sorted[j + 1] = sorted[j];
        }
        sorted[j + 1] = v;
    }
    sum = 0;
    for (i = 0; i < NUM_BINS / 4; i++) {
        sum += sorted[i];
    }
    floorLevel = (uint16_t) (sum / (NUM_BINS / 4));

    // peak = highest bin
    pk = 0;
    for (i = 1; i < NUM_BINS; i++) {
        if (profile[i] > profile[pk]) {
            pk = i;
        }
    }
    peakLevel = profile[pk];
    found = (peakLevel >= floorLevel + BEACON_MIN_RISE) ? TRUE : FALSE;

    // center of mass of everything above half height, walking out from the peak both ways
    half = floorLevel + (peakLevel - floorLevel) / 2;
    left = 0;
    while (left < NUM_BINS / 2 && profile[Wrap(pk - left - 1)] >= half) {
        left++;
    }
    right = 0;
    while (right < NUM_BINS / 2 && profile[Wrap(pk + right + 1)] >= half) {
        right++;
    }
    weightSum = 0;
    weightedPos = 0;
    for (offset = -left; offset <= right; offset++) {
        int32_t w = (int32_t) profile[Wrap(pk + offset)] - half + 1; // +1 so the edge bins still count a little
        weightSum += w;
        weightedPos += w * offset;
    }
    widthDeg = (uint16_t) ((left + right + 1) * 360 / NUM_BINS);

    // bin center in tenths of a bin, then to degrees: bin i covers i*5 to i*5+5 degrees
    {
        int32_t centerTenths = (int32_t) pk * 10 + 5 + (weightedPos * 10) / weightSum;
        int32_t deg = (centerTenths * 360 + (NUM_BINS * 10) / 2) / (NUM_BINS * 10) + BEACON_ANGLE_OFFSET_DEG; // rounded
        while (deg < 0) deg += 360;
        while (deg >= 360) deg -= 360;
        angleDeg = (int16_t) deg;
    }
}

/*******************************************************************************
 * PUBLIC FUNCTIONS                                                            *
 ******************************************************************************/

void Beacon_Init(void)
{
    AD_AddPins(BEACON_PIN);
    scanning = FALSE;
    found = FALSE;
}

uint16_t Beacon_ReadLevel(void)
{
    return (uint16_t) AD_ReadADPin(BEACON_PIN);
}

void Beacon_StartScan(void)
{
    int i;
    for (i = 0; i < NUM_BINS; i++) {
        binSum[i] = 0;
        binCount[i] = 0;
    }
    found = FALSE;
    scanStart = ES_Timer_GetTime();
    scanning = TRUE;
}

uint8_t Beacon_IsScanning(void)
{
    return scanning;
}

uint8_t Beacon_Found(void)
{
    return found;
}

int16_t Beacon_GetAngle(void)
{
    return angleDeg;
}

int16_t Beacon_GetTurnDeg(void)
{
    return (angleDeg <= 180) ? angleDeg : angleDeg - 360;
}

uint16_t Beacon_GetPeakLevel(void)
{
    return peakLevel;
}

uint16_t Beacon_GetFloorLevel(void)
{
    return floorLevel;
}

uint16_t Beacon_GetWidthDeg(void)
{
    return widthDeg;
}

uint8_t CheckBeacon(void)
{
    ES_Event thisEvent;
    uint32_t elapsed;
    int bin;

    if (!scanning) {
        return FALSE;
    }

    elapsed = ES_Timer_GetTime() - scanStart;
    if (elapsed < T_REV_MS) {
        // still spinning, drop this reading into the bin for this angle
        bin = (int) ((elapsed * NUM_BINS) / T_REV_MS);
        binSum[bin] += FilterSample(Beacon_ReadLevel());
        binCount[bin]++;
        return FALSE;
    }

    // one full revolution done
    scanning = FALSE;
    Analyze();
    thisEvent.EventType = BEACON_SCAN_DONE;
    thisEvent.EventParam = found ? (uint16_t) angleDeg : BEACON_NOT_FOUND;
    PostTopHSM(thisEvent);
    return TRUE;
}
