
#ifndef FREERTOS_TSN_NETWORK_SCHEDULER_H
#define FREERTOS_TSN_NETWORK_SCHEDULER_H

#include "FreeRTOS_TSN_NetworkSchedulerBlock.h"
#include "FreeRTOS_TSN_NetworkSchedulerQueue.h"

struct xQUEUE_LIST
{
	struct xNETQUEUE *pxQueue;
	struct xQUEUE_LIST *pxNext;
};

typedef struct xQUEUE_LIST NetworkQueueList_t;

void vNetworkQueueListAdd( NetworkQueueList_t *pxItem );

BaseType_t xNetworkQueueAssignRoot( NetworkNode_t * pxNode );

/* This must be defined by the user */
void vNetworkQueueInit( void );

BaseType_t xNetworkQueueInsertPacketByFilter( const NetworkQueueItem_t * pxItem, UBaseType_t uxTimeout );

BaseType_t xNetworkQueueInsertPacketByName( const NetworkQueueItem_t * pxItem, char * pcQueueName, UBaseType_t uxTimeout );

NetworkQueue_t * xNetworkQueueSchedule( void );

BaseType_t xNetworkQueuePush( NetworkQueue_t * pxQueue, const NetworkQueueItem_t * pxItem, UBaseType_t uxTimeout );

BaseType_t xNetworkQueuePop( NetworkQueue_t * pxQueue, NetworkQueueItem_t * pxItem, UBaseType_t uxTimeout );

#if ( tsnconfigMAX_QUEUE_NAME_LEN != 0 )
	NetworkQueue_t * pxNetworkQueueFindByName( char * pcName, const NetworkQueueItem_t * pxItem );
#endif

#endif /* FREERTOS_TSN_NETWORK_SCHEDULER_H */
