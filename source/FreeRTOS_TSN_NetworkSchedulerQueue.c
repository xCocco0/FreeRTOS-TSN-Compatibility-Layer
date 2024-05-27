
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
		pxQueue->uxTimeout = pdMS_TO_TICKS( tsnconfigDEFAULT_QUEUE_TIMEOUT );
		pxQueue->ePolicy = eSendRecv;
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

