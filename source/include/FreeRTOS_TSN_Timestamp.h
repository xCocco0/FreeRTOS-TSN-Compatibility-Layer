
#ifndef FREERTOS_TSN_TIMESTAMP_H
#define FREERTOS_TSN_TIMESTAMP_H

#include <stdint.h>

struct freertos_timespec
{
    uint32_t tv_sec;
    uint32_t tv_nsec;
};

struct freertos_scm_timestamping {
        struct freertos_timespec ts[3]; /* ts[0] for software timestamps, ts[2] hw timestamps*/
};

void vTimestampAcquire( struct freertos_timespec * ts );

#endif /* FREERTOS_TSN_TIMESTAMP_H */