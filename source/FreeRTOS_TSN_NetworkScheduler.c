
#include "FreeRTOS_TSN_NetworkScheduler.h"
#include "FreeRTOS_TSN_Controller.h"

NetworkNode_t *pxNetworkQueueRoot = NULL;
NetworkQueueList_t *pxNetworkQueueList = NULL;
UBaseType_t uxNumQueues = 0;

BaseType_t prvMatchQueuePolicy( const NetworkQueueItem_t * pxItem, NetworkQueue_t * pxQueue )
{
	switch( pxQueue->ePolicy )
	{
		case eSendRecv:
			return ( pxItem->eEventType == eNetworkTxEvent ) || ( pxItem->eEventType == eNetworkRxEvent );
			break;

		case eSendOnly:
			return ( pxItem->eEventType == eNetworkTxEvent );
			break;

		case eRecvOnly:
			return ( pxItem->eEventType == eNetworkRxEvent );
			break;

		case eIPTaskEvents:
			return pdTRUE;

		default:
			return pdFALSE;
	}
}

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

BaseType_t xNetworkQueueInsertPacketByFilter( const NetworkQueueItem_t * pxItem )
{
	NetworkQueueList_t * pxIterator = pxNetworkQueueList;
	NetworkBufferDescriptor_t * pxNetworkBuffer = ( NetworkBufferDescriptor_t * ) pxItem->pvData;
	
	while( pxIterator != NULL )
	{
		if( pxIterator->pxQueue->fnFilter( pxNetworkBuffer ) && prvMatchQueuePolicy( pxItem, pxIterator->pxQueue ) )
		{
			return xNetworkQueuePush( pxIterator->pxQueue, pxItem );
		}
		
		pxIterator = pxIterator->pxNext;
	}

	return pdFAIL;
}

BaseType_t xNetworkQueueInsertPacketByName( const NetworkQueueItem_t * pxItem, char * pcQueueName )
{
	NetworkQueue_t * pxQueue;

	pxQueue = pxNetworkQueueFindByName( pcQueueName, pxItem );
	
	if( pxQueue != NULL )
	{
		return xNetworkQueuePush( pxQueue, pxItem );
	}

	return pdFALSE;
}

NetworkQueue_t * xNetworkQueueSchedule( void )
{
	if( pxNetworkQueueRoot != NULL )
	{
		return pxNetworkSchedulerCall( pxNetworkQueueRoot );
	}

	return pdFAIL;
}

BaseType_t xNetworkQueuePush( NetworkQueue_t * pxQueue, const NetworkQueueItem_t * pxItem)
{
	#if ( tsnconfigINCLUDE_QUEUE_EVENT_CALLBACKS != tsnconfigDISABLE )
		pxQueue->fnOnPush( ( NetworkBufferDescriptor_t * ) pxItem->pvData );
	#endif

    if( xQueueSendToBack( pxQueue->xQueue, ( void * ) pxItem, pxQueue->uxTimeout) == pdPASS)
	{
		xNotifyController();
		return pdPASS;
	}
	return pdFAIL;
}

BaseType_t xNetworkQueuePop( NetworkQueue_t * pxQueue, NetworkQueueItem_t * pxItem )
{
	if( xQueueReceive( pxQueue->xQueue, pxItem, 0 ) != pdPASS )
	{
		/* queue empty */
		return pdFAIL;
	}
	
	#if ( tsnconfigINCLUDE_QUEUE_EVENT_CALLBACKS != tsnconfigDISABLE )
		pxQueue->fnOnPop( ( NetworkBufferDescriptor_t * ) pxItem->pvData );
	#endif

	return pdPASS;
}

NetworkQueue_t * pxNetworkQueueFindByName( char * pcName, const NetworkQueueItem_t * pxItem )
{
	NetworkQueueList_t *pxIterator = pxNetworkQueueList;
	
	while( pxIterator != NULL )
	{
		if( prvMatchQueuePolicy( pxItem, pxIterator->pxQueue ) )
		{
			if( strncmp( pcName, pxIterator->pxQueue->cName, tsnconfigMAX_QUEUE_NAME_LEN ) == 0 )
			{
				return pxIterator->pxQueue;
			}
		}
		pxIterator = pxIterator->pxNext;
	}

	return NULL;
}
