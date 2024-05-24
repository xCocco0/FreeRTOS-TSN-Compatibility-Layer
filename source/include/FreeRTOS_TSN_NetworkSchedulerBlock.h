
#ifndef FREERTOS_TSN_NETWORK_SCHEDULER_BLOCK_H
#define FREERTOS_TSN_NETWORK_SCHEDULER_BLOCK_H

#include "FreeRTOS_TSN_NetworkSchedulerQueue.h"

struct xNETQUEUE_NODE
{
	uint8_t ucNumChildren;
	void *pvScheduler;
	struct xNETQUEUE *pxQueue;
	struct xNETQUEUE_NODE *pxNext[];
};

typedef struct xNETQUEUE_NODE NetworkNode_t;

typedef NetworkQueue_t * ( * SelectQueueFunction_t ) ( NetworkNode_t* pxNode );

typedef BaseType_t ( * ReadyQueueFunction_t ) ( NetworkNode_t * pxNode );

struct xSCHEDULER_GENERIC
{
	uint16_t usSize;
	struct xNETQUEUE_NODE *pxOwner;
	SelectQueueFunction_t fnSelect;
	ReadyQueueFunction_t fnReady;
	char ucAttributes[];
};

#if ( configSUPPORT_DYNAMIC_ALLOCATION != 0 )

NetworkNode_t * pxNetworkNodeCreate( BaseType_t xNumChildren );

void vNetworkNodeRelease( NetworkNode_t * pxNode );

void * pvNetworkSchedulerGenericCreate( NetworkNode_t * pxNode, uint16_t usSize);

void vNetworkSchedulerGenericRelease( void * pvSched );

#endif

NetworkQueue_t * pxNetworkSchedulerCall( NetworkNode_t * pxNode );

NetworkBufferDescriptor_t * pxPeekNextPacket( NetworkNode_t * pxNode);

TickType_t uxNetworkQueueGetTicksUntilWakeup( void );

void vNetworkQueueAddWakeupEvent( TickType_t uxTime );

#define netschedCALL_SELECT_FROM_NODE( pxNode ) \
	( ( ( struct xSCHEDULER_GENERIC * ) pxNode->pvScheduler )->fnSelect( pxNode ) )

#define netschedCALL_READY_FROM_NODE( pxNode ) \
	( ( ( struct xSCHEDULER_GENERIC * ) pxNode->pvScheduler )->fnReady( pxNode ) )

#endif /* FREERTOS_TSN_NETWORK_SCHEDULER_H */
