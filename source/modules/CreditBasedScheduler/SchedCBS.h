
#ifndef SCHED_CBS_H
#define SCHED_CBS_H

#include "FreeRTOS_TSN_NetworkSchedulerBlock.h"

#define netschedCBS_DEFAULT_BANDWIDTH ( 1 << 20 )
#define netschedCBS_DEFAULT_MAXCREDIT ( 1536 * 2 ) // max burst = 2 frames

NetworkNode_t * pxNetworkNodeCreateCBS( UBaseType_t uxBandwidth, UBaseType_t uxMaxCredit );

#endif
