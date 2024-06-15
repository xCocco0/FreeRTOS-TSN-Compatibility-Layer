/**
 * @file FreeRTOS_TSN_NetworkScheduler.c
 * @brief This file contains the implementation of the network scheduler for FreeRTOS TSN Compatibility Layer.
 */

#include "FreeRTOS_TSN_NetworkScheduler.h"
#include "FreeRTOS_TSN_Controller.h"

NetworkNode_t *pxNetworkQueueRoot = NULL;
NetworkQueueList_t *pxNetworkQueueList = NULL;
UBaseType_t uxNumQueues = 0;

/**
 * @brief Matches the filtering policy of a network queue with a network queue item.
 *
 * @param pxItem The network queue item to match.
 * @param pxQueue The network queue to match against.
 * @return pdTRUE if the network queue item matches the filtering policy of the network queue, pdFALSE otherwise.
 */
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

/**
 * @brief Adds a network queue to the network queue list.
 *
 * @param pxItem The network queue to add.
 */
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

/**
 * @brief Assigns the root network node.
 *
 * @param pxNode The network node to assign as the root.
 * @return pdPASS if the root network node is successfully assigned, pdFAIL otherwise.
 */
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
/**
 * @brief Iterate over the list of network queues defined in vNetworkQueueInit and find
 * a match based on the queues' filtering policy. If more queues match the
 * filter, the one with the highest IPV is chosen.

 * @param pxItem The network queue item to insert.
 * @param uxTimeout The timeout value for the insertion operation.
 * @return pdPASS if the network queue item is successfully inserted, pdFAIL otherwise.
 */
BaseType_t xNetworkQueueInsertPacketByFilter( const NetworkQueueItem_t * pxItem, UBaseType_t uxTimeout )
{
	NetworkQueueList_t * pxIterator = pxNetworkQueueList;
	NetworkBufferDescriptor_t * pxNetworkBuffer = ( NetworkBufferDescriptor_t * ) pxItem->pxBuf;
	NetworkQueue_t * pxChosenQueue = NULL;
	
	while( pxIterator != NULL )
	{
		if( pxIterator->pxQueue->fnFilter( pxNetworkBuffer ) && prvMatchQueuePolicy( pxItem, pxIterator->pxQueue ) )
		{
			if( pxChosenQueue != NULL )
			{
				if( pxIterator->pxQueue->uxIPV <= pxChosenQueue->uxIPV ) {
					pxIterator = pxIterator->pxNext;
					continue;
				}	
			}

			pxChosenQueue = pxIterator->pxQueue;
		}
		pxIterator = pxIterator->pxNext;
	}
	
	if( pxChosenQueue != NULL )
	{
		return xNetworkQueuePush( pxChosenQueue, pxItem, uxTimeout );
	}
	else
	{
		return pdFAIL;
	}
}

/**
 * @brief Inserts a network queue item into a network queue based on the queue name.
 *
 * @param pxItem The network queue item to insert.
 * @param pcQueueName The name of the network queue to insert into.
 * @param uxTimeout The timeout value for the insertion operation.
 * @return pdPASS if the network queue item is successfully inserted, pdFAIL otherwise.
 */
BaseType_t xNetworkQueueInsertPacketByName( const NetworkQueueItem_t * pxItem, char * pcQueueName, UBaseType_t uxTimeout )
{
	NetworkQueue_t * pxQueue;

	pxQueue = pxNetworkQueueFindByName( pcQueueName, pxItem );
	
	if( pxQueue != NULL )
	{
		return xNetworkQueuePush( pxQueue, pxItem, uxTimeout );
	}

	return pdFALSE;
}

/**
 * @brief Schedules the network queues and returns the chosen network queue.
 *
 * @return The chosen network queue, or pdFAIL if no network queue is available.
 */
NetworkQueue_t * xNetworkQueueSchedule( void )
{
	if( pxNetworkQueueRoot != NULL )
	{
		return pxNetworkSchedulerCall( pxNetworkQueueRoot );
	}

	return pdFAIL;
}

/**
 * @brief Pushes a network queue item into a network queue.
 *
 * @param pxQueue The network queue to push the item into.
 * @param pxItem The network queue item to push.
 * @param uxTimeout The timeout value for the push operation.
 * @return pdPASS if the network queue item is successfully pushed, pdFAIL otherwise.
 */
BaseType_t xNetworkQueuePush( NetworkQueue_t * pxQueue, const NetworkQueueItem_t * pxItem, UBaseType_t uxTimeout )
{
	#if ( tsnconfigINCLUDE_QUEUE_EVENT_CALLBACKS != tsnconfigDISABLE )
		pxQueue->fnOnPush( ( NetworkBufferDescriptor_t * ) pxItem->pvData );
	#endif

    if( xQueueSendToBack( pxQueue->xQueue, ( void * ) pxItem, uxTimeout) == pdPASS)
	{
		#if ( tsnconfigCONTROLLER_HAS_DYNAMIC_PRIO != tsnconfigDISABLE )
			xTSNControllerUpdatePriority( pxQueue->uxIPV );
		#endif
		xNotifyController();
		return pdPASS;
	}
	return pdFAIL;
}

/**
 * @brief Pops a network queue item from a network queue.
 *
 * @param pxQueue The network queue to pop the item from.
 * @param pxItem The network queue item to pop.
 * @param uxTimeout The timeout value for the pop operation.
 * @return pdPASS if a network queue item is successfully popped, pdFAIL otherwise.
 */
BaseType_t xNetworkQueuePop( NetworkQueue_t * pxQueue, NetworkQueueItem_t * pxItem, UBaseType_t uxTimeout )
{
	if( xQueueReceive( pxQueue->xQueue, pxItem, uxTimeout ) != pdPASS )
	{
		/* queue empty */
		return pdFAIL;
	}
	
	#if ( tsnconfigINCLUDE_QUEUE_EVENT_CALLBACKS != tsnconfigDISABLE )
		pxQueue->fnOnPop( ( NetworkBufferDescriptor_t * ) pxItem->pvData );
	#endif

	return pdPASS;
}

#if ( tsnconfigMAX_QUEUE_NAME_LEN != 0 )
	/**
	 * @brief Finds a network queue by its name.
	 *
	 * @param pcName The name of the network queue to find.
	 * @param pxItem The network queue item to match the filtering policy.
	 * @return The network queue if found, NULL otherwise.
	 */
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
#endif
