#ifndef __NFCV_WORKER_H__
#define __NFCV_WORKER_H__

typedef struct Sensor
{
    unsigned char uid[8]; // FreeStyle Libre serial number
    unsigned char oldUid[8];
    unsigned char nextTrend; // number of next trend block to read
    unsigned char oldNextTrend;
    unsigned char nextHistory; // number of next history block to read
    unsigned char oldNextHistory;
    unsigned char minutesSinceStart; // minutes since start of sensor
    unsigned short oldMinutesSinceStart;
    unsigned char fram[344]; // buffer for Freestyle Libre FRAM data
} S_sensor_t;

typedef enum
{
    E_NOTYETSTARTED = 1,
    E_STARTING,
    E_READY,
    E_EXPIRED,
    E_SHUTDOWN,
    E_FAILURE,
} E_SENSOR_STA;

void workCycle(uint8_t);
#endif
