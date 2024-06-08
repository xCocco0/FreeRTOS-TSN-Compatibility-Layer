
#include <string.h>

#include "FreeRTOS_TSN_NetworkSchedulerQueue.h"
#include "FreeRTOS_TSN_NetworkScheduler.h"

BaseType_t prvDefaultPacketHandler( NetworkBufferDescriptor_t * pxBuf )
{
	return pdPASS;
}

BaseType_t prvAlwaysTrue( NetworkBufferDescriptor_t * pxBuf )
{
	return pdTRUE;
}

#if ( configSUPPORT_DYNAMIC_ALLOCATION != 0 )

NetworkQueue_t * pxNetworkQueueMalloc()
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

NetworkQueue_t * pxNetworkQueueCreate( eQueuePolicy_t ePolicy, UBaseType_t uxIPV, char * cName, FilterFunction_t fnFilter )
{
	NetworkQueue_t * pxQueue = pxNetworkQueueMalloc();

	if( cName != NULL )
	{
		strcpy( (char *) &pxQueue->cName, cName );
	}
	else
	{
		pxQueue->cName[0] = '\0';
	}

	if( fnFilter != NULL )
	{
		pxQueue->fnFilter = fnFilter;
	}
	else
	{
		pxQueue->fnFilter = prvAlwaysTrue;
	}

	pxQueue->ePolicy = ePolicy;
	pxQueue->uxIPV = uxIPV;
	
	return pxQueue;
}

void vNetworkQueueFree( NetworkQueue_t * pxQueue )
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
