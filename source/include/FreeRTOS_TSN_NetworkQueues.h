
#ifndef FREERTOS_TSN_NETWORKQUEUES_H
#define FREERTOS_TSN_NETWORKQUEUES_H

#include "FreeRTOS.h"
#include "queue.h"

#include "FreeRTOS_TSN_Filtering.h"

#define netqueueDEFAULT_QUEUE_TIMEOUT pdMS_TO_TICKS( 50U )


struct xNETQUEUE {
	QueueHandle_t xQueue;
	UBaseType_t uxIPV;
	char ucName[32];
	FilterFunction_t fnFilter;
	TickType_t xTimeout;
};

typedef struct xNETQUEUE NetworkQueue_t;

struct xNETQUEUE_NODE
{
	uint8_t ucNumChildren;
	void *pvScheduler;
	struct xNETQUEUE *pxQueue;
	struct xNETQUEUE_NODE *pxNext[];
};

typedef struct xNETQUEUE_NODE NetworkQueueNode_t;

struct xQUEUE_LIST
{
	struct xNETQUEUE *pxQueue;
	struct xQUEUE_LIST *pxNext;
};

typedef struct xQUEUE_LIST NetworkQueueList_t;

#if ( configSUPPORT_DYNAMIC_ALLOCATION != 0 )

NetworkQueueNode_t * pxNetworkQueueNodeCreate( BaseType_t xNumChildren );

void vNetworkQueueNodeRelease( NetworkQueueNode_t *pxNode );

NetworkQueue_t * pxNetworkQueueCreate();

NetworkQueue_t* pxNetworkQueueFromIPEventQueue();

void vNetworkQueueRelease( NetworkQueue_t *pxQueue );

#endif

BaseType_t xNetworkQueueAssignRoot( NetworkQueueNode_t *pxNode );

extern void vNetworkQueueInit();

NetworkQueue_t * pxNetworkQueueSearchMatch( NetworkBufferDescriptor_t *pxNetworkBuffer );

BaseType_t xNetworkQueueInsertPacket( NetworkBufferDescriptor_t * pxNetworkBuffer );

NetworkBufferDescriptor_t * pxNetworkQueueRetrievePacket( void );

BaseType_t xNetworkQueuePush( NetworkQueue_t * pxQueue, void * pvObject);

NetworkBufferDescriptor_t * pxNetworkQueuePop( NetworkQueue_t * pxQueue );

#endif /* FREERTOS_TSN_NETWORKQUEUES_H */
