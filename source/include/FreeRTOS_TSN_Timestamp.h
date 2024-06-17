#ifndef FREERTOS_TSN_TIMESTAMP_H
#define FREERTOS_TSN_TIMESTAMP_H

#include <stdint.h>

#include "FreeRTOS_TSN_Timebase.h"

struct freertos_scm_timestamping
{
    struct freertos_timespec ts[ 3 ];   /* ts[0] for software timestamps, ts[2] hw timestamps*/
};

void vTimestampAcquireSoftware( struct freertos_timespec * ts );

#endif /* FREERTOS_TSN_TIMESTAMP_H */
