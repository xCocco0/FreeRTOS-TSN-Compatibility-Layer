
#ifndef FREERTOS_TSN_NETWORK_SCHEDULER_H
#define FREERTOS_TSN_NETWORK_SCHEDULER_H

#include "FreeRTOS_TSN_NetworkQueues.h"

#define netqueueSCHEDULER_RR    ( 1 )
#define netqueueSCHEDULER_PRIO  ( 2 )
#define netqueueSHAPER_CBS      ( 3 )
#define netqueueSHAPER_FIFO     ( 4 )

typedef NetworkQueue_t * ( * SelectQueueFunction_t ) ( NetworkQueueNode_t* pxSched );

typedef BaseType_t ( * ReadyQueueFunction_t ) ( NetworkQueueNode_t * pxSched );

struct xSCHEDULER_GENERIC
{
	uint16_t usSize;
	struct xNETQUEUE_NODE *pxOwner;
	SelectQueueFunction_t fnSelect;
	ReadyQueueFunction_t fnReady;
	char ucAttributes[];
};

void * pvNetworkSchedulerGenericCreate( NetworkQueueNode_t * pxNode, uint16_t usSize);

void vNetworkSchedulerGenericRelease( void * pvSched );

NetworkQueue_t * pxNetworkSchedulerCall( NetworkQueueNode_t * pxNode );

#define netschedCALL_SELECT_FROM_NODE( pxNode ) \
	( ( ( struct xSCHEDULER_GENERIC * ) pxNode->pvScheduler )->fnSelect( pxNode ) )

#define netschedCALL_READY_FROM_NODE( pxNode ) \
	( ( ( struct xSCHEDULER_GENERIC * ) pxNode->pvScheduler )->fnReady( pxNode ) )


NetworkQueueNode_t * pxNetworkQueueNodeCreateFIFO();

#endif /* FREERTOS_TSN_NETWORK_SCHEDULER_H */
