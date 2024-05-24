
#include "FreeRTOS_TSN_NetworkScheduler.h"
#include "FreeRTOS_TSN_Controller.h"

NetworkNode_t *pxNetworkQueueRoot = NULL;
NetworkQueueList_t *pxNetworkQueueList = NULL;
UBaseType_t uxNumQueues = 0;

void vNetworkQueueListAdd( NetworkQueueList_t *pxItem )
{	
	uxNumQueues += 1;

	if( pxNetworkQueueList == NULL )
	{
		pxNetworkQueueList = pxItem;
	}
	else
	{
		NetworkQueueList_t *pxIndex = pxNetworkQueueList;
		while( pxIndex->pxNext != NULL)
		{
			pxIndex = pxIndex->pxNext;
		}
		pxIndex->pxNext = pxItem;
	}
}

BaseType_t xNetworkQueueAssignRoot( NetworkNode_t *pxNode )
{
	if( pxNetworkQueueRoot == NULL )
	{
		pxNetworkQueueRoot = pxNode;
		return pdPASS;
	}
	else
	{
		return pdFAIL;
	}
}

BaseType_t xNetworkQueueInsertPacket( const IPStackEvent_t * pxEvent )
{
	NetworkQueueList_t * pxIterator = pxNetworkQueueList;
	NetworkBufferDescriptor_t * pxNetworkBuffer = ( NetworkBufferDescriptor_t * ) pxEvent->pvData;

	while( pxIterator != NULL )
	{
		if( pxIterator->pxQueue->fnFilter( pxNetworkBuffer ) )
		{
			return xNetworkQueuePush( pxIterator->pxQueue, pxEvent );
		}
		
		pxIterator = pxIterator->pxNext;
	}

	return pdFAIL;
}

BaseType_t xNetworkQueueInsertPacketByName( const IPStackEvent_t * pxEvent, char * pcQueueName )
{
	NetworkQueue_t * pxQueue;

	pxQueue = pxNetworkQueueFindByName( pcQueueName );
	
	if( pxQueue != NULL )
	{
		return xNetworkQueuePush( pxQueue, pxEvent );
	}

	return pdFALSE;
}

BaseType_t xNetworkQueueRetrievePacket( IPStackEvent_t * pxEvent )
{
	NetworkQueue_t * pxChosenQueue;

	if( pxNetworkQueueRoot != NULL )
	{
		pxChosenQueue = pxNetworkSchedulerCall( pxNetworkQueueRoot );

		if( pxChosenQueue != NULL )
		{
			return xNetworkQueuePop( pxChosenQueue, pxEvent );
		}
	}

	return pdFAIL;
}

BaseType_t xNetworkQueuePush( NetworkQueue_t * pxQueue, const IPStackEvent_t * pxEvent)
{
	pxQueue->fnOnPush( ( NetworkBufferDescriptor_t * ) pxEvent->pvData );

    if( xQueueSendToBack( pxQueue->xQueue, ( void * ) pxEvent, pxQueue->uxTimeout) == pdPASS)
	{
		xNotifyController();
		return pdPASS;
	}
	return pdFAIL;
}

BaseType_t xNetworkQueuePop( NetworkQueue_t * pxQueue, IPStackEvent_t * pxEvent )
{
	if( xQueueReceive( pxQueue->xQueue, pxEvent, 0 ) != pdPASS )
	{
		/* queue empty */
		return pdFAIL;
	}
	
	pxQueue->fnOnPop( ( NetworkBufferDescriptor_t * ) pxEvent->pvData );		

	return pdPASS;
}

NetworkQueue_t * pxNetworkQueueFindByName( char * pcName )
{
	NetworkQueueList_t *pxIterator = pxNetworkQueueList;
	
	while( pxIterator != NULL )
	{
		if( strncmp( pcName, pxIterator->pxQueue->cName, netqueueMAX_QUEUE_NAME_LEN ) == 0 )
		{
			return pxIterator->pxQueue;
		}
	}

	return NULL;
}
