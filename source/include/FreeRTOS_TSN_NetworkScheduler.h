
#ifndef FREERTOS_TSN_NETWORK_SCHEDULER_H
#define FREERTOS_TSN_NETWORK_SCHEDULER_H

#include "FreeRTOS_TSN_NetworkQueues.h"

#define netqueueSCHEDULER_RR    ( 1 )
#define netqueueSCHEDULER_PRIO  ( 2 )
#define netqueueSHAPER_CBS      ( 3 )
#define netqueueSHAPER_FIFO     ( 4 )

typedef NetworkQueue_t * ( * SelectQueueFunction_t ) ( NetworkQueueNode_t* pxNode );

typedef BaseType_t ( * ReadyQueueFunction_t ) ( NetworkQueueNode_t * pxNode );

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

NetworkBufferDescriptor_t * pxPeekNextPacket( NetworkQueueNode_t * pxNode);

#define netschedCALL_SELECT_FROM_NODE( pxNode ) \
	( ( ( struct xSCHEDULER_GENERIC * ) pxNode->pvScheduler )->fnSelect( pxNode ) )

#define netschedCALL_READY_FROM_NODE( pxNode ) \
	( ( ( struct xSCHEDULER_GENERIC * ) pxNode->pvScheduler )->fnReady( pxNode ) )

/*---------------------------------------------------------------------------*/

NetworkQueueNode_t * pxNetworkQueueNodeCreateFIFO();

#define netschedCBS_DEFAULT_BANDWIDTH ( 1 << 20 )
#define netschedCBS_DEFAULT_MAXCREDIT ( 1536 * 2 ) // max burst = 2 frames

NetworkQueueNode_t * pxNetworkQueueNodeCreateCBS( UBaseType_t uxBandwidth, UBaseType_t uxMaxCredit );


#endif /* FREERTOS_TSN_NETWORK_SCHEDULER_H */
