
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
		pxQueue->xQueue = xQueueCreate( ipconfigEVENT_QUEUE_LENGTH, sizeof( NetworkQueueItem_t ) );
		configASSERT( pxQueue->xQueue != NULL );
		pxQueue->ePolicy = eSendRecv;
		pxQueue->uxIPV = 0;
		#if ( tsnconfigINCLUDE_QUEUE_EVENT_CALLBACKS != tsnconfigDISABLE )
			pxQueue->fnOnPop = prvDefaultPacketHandler;
			pxQueue->fnOnPush = prvDefaultPacketHandler;
		#endif
		
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

UBaseType_t uxNetworkQueuePacketsWaiting( NetworkQueue_t * pxQueue )
{
	return uxQueueMessagesWaiting( pxQueue->xQueue );
}

BaseType_t xNetworkQueueIsEmpty( NetworkQueue_t * pxQueue )
{
	return uxQueueMessagesWaiting( pxQueue->xQueue ) == 0 ? pdTRUE : pdFALSE;
}
