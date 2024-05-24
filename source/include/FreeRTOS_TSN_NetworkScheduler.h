
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

extern void vNetworkQueueInit();

BaseType_t xNetworkQueueInsertPacket( const IPStackEvent_t * pxEvent );

BaseType_t xNetworkQueueInsertPacketByName( const IPStackEvent_t * pxEvent, char * pcQueueName );

BaseType_t xNetworkQueueRetrievePacket( IPStackEvent_t * pxEvent );

BaseType_t xNetworkQueuePush( NetworkQueue_t * pxQueue, const IPStackEvent_t * pxEvent);

BaseType_t xNetworkQueuePop( NetworkQueue_t * pxQueue, IPStackEvent_t * pxEvent );

NetworkQueue_t * pxNetworkQueueFindByName( char * pcName );

#endif /* FREERTOS_TSN_NETWORK_SCHEDULER_H */
