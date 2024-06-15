
#include "FreeRTOS_TSN_Timestamp.h"

void vTimestampAcquire( struct freertos_timespec * ts )
{
    ts->tv_sec = (1 << 31) | 1;
    ts->tv_nsec = (1 << 31) | 3;
    /*TODO*/
}