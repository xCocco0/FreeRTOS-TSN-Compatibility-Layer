
#include <string.h>

#include "FreeRTOS_TSN_NetworkSchedulerQueue.h"
#include "FreeRTOS_TSN_NetworkScheduler.h"

BaseType_t prvDefaultPacketHandler( NetworkBufferDescriptor_t * pxBuf )
{
	return pdPASS;
}

#if ( configSUPPORT_DYNAMIC_ALLOCATION != 0 )

NetworkQueue_t* pxNetworkQueueCreate()
{
	NetworkQueue_t *pxQueue = pvPortMalloc( sizeof( NetworkQueue_t ) );
	NetworkQueueList_t *pxNode;

	if( pxQueue != NULL )
	{
		memset( pxQueue, '\0', sizeof( NetworkQueue_t ) );
		pxQueue->xQueue = xQueueCreate( ipconfigEVENT_QUEUE_LENGTH, sizeof( IPStackEvent_t ) );
		configASSERT( pxQueue->xQueue != NULL );
		pxQueue->uxTimeout = netqueueDEFAULT_QUEUE_TIMEOUT;
		pxQueue->ePolicy = eSendRecv;
		pxQueue->fnOnPop = prvDefaultPacketHandler;
		pxQueue->fnOnPush = prvDefaultPacketHandler;
		
		pxNode = pvPortMalloc( sizeof( NetworkQueueList_t ) );
		pxNode->pxQueue = pxQueue;
		pxNode->pxNext = NULL;

		vNetworkQueueListAdd( pxNode );
	}

	return pxQueue;
}

NetworkQueue_t* pxNetworkQueueFromIPEventQueue()
{
	NetworkQueue_t *pxQueue = pvPortMalloc( sizeof( NetworkQueue_t ) );
	NetworkQueueList_t *pxNode;

	if( pxQueue != NULL )
	{
		memset( pxQueue, '\0', sizeof( NetworkQueue_t ) );
		pxQueue->xQueue = xNetworkEventQueue;
		pxQueue->uxTimeout = netqueueDEFAULT_QUEUE_TIMEOUT;
		pxQueue->ePolicy = eSendRecv;
		pxQueue->fnOnPop = prvDefaultPacketHandler;
		pxQueue->fnOnPush = prvDefaultPacketHandler;

		pxNode = pvPortMalloc( sizeof( NetworkQueueList_t ) );
		pxNode->pxQueue = pxQueue;
		pxNode->pxNext = NULL;

		vNetworkQueueListAdd( pxNode );
	}

	return pxQueue;
}

void vNetworkQueueRelease( NetworkQueue_t *pxQueue )
{
	vQueueDelete( pxQueue->xQueue );
	vPortFree( pxQueue );
}

#endif

