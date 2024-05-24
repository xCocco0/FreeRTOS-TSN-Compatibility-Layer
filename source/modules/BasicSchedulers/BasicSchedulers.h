
#ifndef BASIC_SCHEDULERS_H
#define BASIC_SCHEDULERS_H

#include "FreeRTOS_TSN_NetworkSchedulerBlock.h"

NetworkNode_t * pxNetworkNodeCreateFIFO( void );

NetworkNode_t * pxNetworkNodeCreateRR( BaseType_t uxNumChildren );

NetworkNode_t * pxNetworkNodeCreatePrio( BaseType_t uxNumChildren );

#endif /* BASIC_SCHEDULERS_H */
