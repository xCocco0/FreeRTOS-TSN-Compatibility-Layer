/**
 * @file FreeRTOS_TSN_NetworkSchedulerBlock.c
 * @brief Implementation of the FreeRTOS TSN Network Scheduler Block.
 *
 * This file contains the implementation of the FreeRTOS TSN Network Scheduler Block.
 * It provides functions for creating and releasing network nodes, linking queues and children to a node,
 * selecting the first node, checking if a node is always ready, and calling the network scheduler.
 * It also includes functions for peeking the next packet, getting the ticks until wakeup, and adding a wakeup event.
 */

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "FreeRTOS_TSN_NetworkSchedulerBlock.h"

TickType_t uxNextWakeup = 0;

/**
 * @brief Checks if a network node is always ready.
 *
 * This function is used to check if a network node is always ready for scheduling.
 *
 * @param pxNode Pointer to the network node.
 *
 * @return pdTRUE if the node is always ready, pdFALSE otherwise.
 */
BaseType_t prvAlwaysReady( NetworkNode_t * pxNode )
{
	return pdTRUE;
}

/**
 * @brief Selects the first network node.
 *
 * This function is used to select the first network node for scheduling.
 *
 * @param pxNode Pointer to the network node.
 *
 * @return Pointer to the selected network queue.
 */
NetworkQueue_t * prvSelectFirst( NetworkNode_t * pxNode )
{
	return pxNetworkSchedulerCall( pxNode->pxNext[0] );
}

#if ( configSUPPORT_DYNAMIC_ALLOCATION != 0 )

/**
 * @brief Creates a network node.
 *
 * This function is used to create a network node with the specified number of children.
 *
 * @param uxNumChildren Number of children for the network node.
 *
 * @return Pointer to the created network node.
 */
NetworkNode_t * pxNetworkNodeCreate( UBaseType_t uxNumChildren )
{
	NetworkNode_t *pxNode;
	UBaseType_t uxSpaceRequired = sizeof( NetworkNode_t ) + uxNumChildren * sizeof( NetworkNode_t * );

	pxNode = pvPortMalloc( uxSpaceRequired );
	
	if( pxNode != NULL )
	{
		memset( pxNode, '\0', uxSpaceRequired );
		pxNode->ucNumChildren = uxNumChildren;
	}

	return pxNode;
}

/**
 * @brief Releases a network node.
 *
 * This function is used to release a network node.
 *
 * @param pxNode Pointer to the network node to be released.
 */
void vNetworkNodeRelease( NetworkNode_t *pxNode )
{
	vPortFree( pxNode );
}

/**
 * @brief Creates a generic network scheduler.
 *
 * This function is used to create a generic network scheduler for the specified network node and size.
 *
 * @param pxNode Pointer to the network node.
 * @param usSize Size of the network scheduler.
 *
 * @return Pointer to the created network scheduler.
 */
void * pvNetworkSchedulerGenericCreate( NetworkNode_t * pxNode, uint16_t usSize )
{
	if( usSize >= sizeof( struct xSCHEDULER_GENERIC ) )
	{
		struct xSCHEDULER_GENERIC * pxSched = pvPortMalloc( usSize );
		pxSched->usSize = usSize;
		pxSched->pxOwner = pxNode;
		pxSched->fnSelect = prvSelectFirst;
		pxSched->fnReady = prvAlwaysReady;
		pxNode->pvScheduler = pxSched;

		return pxSched;
	}
	else
	{
		return NULL;
	}
}

/**
 * @brief Releases a generic network scheduler.
 *
 * This function is used to release a generic network scheduler.
 *
 * @param pvSched Pointer to the network scheduler to be released.
 */
void vNetworkSchedulerGenericRelease( void * pvSched )
{
	struct xSCHEDULER_GENERIC * pxSched = ( struct xSCHEDULER_GENERIC * ) pvSched;
	pxSched->pxOwner->pvScheduler = NULL;
	vPortFree( pvSched );
}

#endif

/**
 * @brief Links a network queue to a network node.
 *
 * This function is used to link a network queue to a network node.
 *
 * @param pxNode Pointer to the network node.
 * @param pxQueue Pointer to the network queue.
 *
 * @return pdPASS if the link is successful, pdFAIL otherwise.
 */
BaseType_t xNetworkSchedulerLinkQueue( NetworkNode_t * pxNode, NetworkQueue_t * pxQueue )
{
	if( pxNode != NULL )
	{
		pxNode->pxQueue = pxQueue;
		return pdPASS;
	}

	return pdFAIL;
}

/**
 * @brief Links a child network node to a parent network node.
 *
 * This function is used to link a child network node to a parent network node at the specified position.
 *
 * @param pxNode Pointer to the parent network node.
 * @param pxChild Pointer to the child network node.
 * @param uxPosition Position at which to link the child network node.
 *
 * @return pdPASS if the link is successful, pdFAIL otherwise.
 */
BaseType_t xNetworkSchedulerLinkChild( NetworkNode_t * pxNode, NetworkNode_t * pxChild, size_t uxPosition )
{
	if( pxNode != NULL )
	{
		if( uxPosition < pxNode->ucNumChildren && pxNode->pxQueue == NULL )
		{
			pxNode->pxNext[ uxPosition ] = pxChild;
			return pdPASS;
		}
	}
	
	return pdFAIL;
}

/**
 * @brief Calls the network scheduler for a network node.
 *
 * This function is used to call the network scheduler for a network node.
 *
 * @param pxNode Pointer to the network node.
 *
 * @return Pointer to the selected network queue.
 */
NetworkQueue_t * pxNetworkSchedulerCall( NetworkNode_t * pxNode )
{
	NetworkQueue_t * pxResult = NULL;

	if( netschedCALL_READY_FROM_NODE( pxNode ) == pdTRUE )
	{
		/* scheduler is ready */

		if( pxNode->pxQueue != NULL )
		{
			/* terminal node */

			if( ! xNetworkQueueIsEmpty( pxNode->pxQueue ) )
			{
				/* queue not empty */
				pxResult = pxNode->pxQueue;
			}
		}
		else
		{
			/* other node to recurse in */
			pxResult = netschedCALL_SELECT_FROM_NODE( pxNode );
		}
	}

	return pxResult;
}

/**
 * @brief Peeks the next packet in a network node's queue.
 *
 * This function is used to peek the next packet in a network node's queue.
 *
 * @param pxNode Pointer to the network node.
 *
 * @return Pointer to the next packet, or NULL if the queue is empty.
 */
NetworkBufferDescriptor_t * pxPeekNextPacket( NetworkNode_t * pxNode )
{
    NetworkQueueItem_t xItem;

    if( xQueuePeek( pxNode->pxQueue->xQueue, &xItem, 0) == pdTRUE )
    {
        return ( NetworkBufferDescriptor_t * ) xItem.pvData;
    }
    else
    {
        return NULL;
    }
}

/**
 * @brief Gets the ticks until the next wakeup event.
 *
 * This function is used to get the number of ticks until the next wakeup event.
 *
 * @return Number of ticks until the next wakeup event.
 */
TickType_t uxNetworkQueueGetTicksUntilWakeup( void )
{
	TickType_t uxNow = xTaskGetTickCount();
	if( uxNow < uxNextWakeup )
	{
		return uxNextWakeup - uxNow;
	}
	else
	{
		return portMAX_DELAY;
	}
}

/**
 * @brief Adds a wakeup event to the network queue.
 *
 * This function is used to add a wakeup event to the network queue at the specified time.
 *
 * @param uxTime Time at which to add the wakeup event.
 */
void vNetworkQueueAddWakeupEvent( TickType_t uxTime )
{
	TickType_t uxNow = xTaskGetTickCount();

	if( uxTime < uxNextWakeup || uxNextWakeup <= uxNow )
	{
		uxNextWakeup = uxTime;
	}
}
