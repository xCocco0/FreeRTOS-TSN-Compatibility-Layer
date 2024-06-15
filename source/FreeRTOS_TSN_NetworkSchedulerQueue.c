/**
 * @file FreeRTOS_TSN_NetworkSchedulerQueue.c
 * @brief Implementation of the FreeRTOS TSN Network Scheduler Queue module.
 *
 * This file contains the implementation of the Network Scheduler Queue module
 * for the FreeRTOS TSN Compatibility Layer. It provides functions for creating,
 * managing, and freeing network queues.
 */

#include <string.h>

#include "FreeRTOS_TSN_NetworkSchedulerQueue.h"
#include "FreeRTOS_TSN_NetworkScheduler.h"

/**
 * @brief Default packet handler function.
 *
 * This function is used as the default packet handler for network buffers.
 * It simply returns pdPASS.
 *
 * @param pxBuf The network buffer descriptor.
 * @return pdPASS.
 */
BaseType_t prvDefaultPacketHandler( NetworkBufferDescriptor_t * pxBuf )
{
	return pdPASS;
}

/**
 * @brief Function that always returns pdTRUE.
 *
 * This function is used as a filter function that always returns pdTRUE.
 * It is used when no filter function is provided for a network queue.
 *
 * @param pxBuf The network buffer descriptor.
 * @return pdTRUE.
 */
BaseType_t prvAlwaysTrue( NetworkBufferDescriptor_t * pxBuf )
{
	return pdTRUE;
}

#if ( configSUPPORT_DYNAMIC_ALLOCATION != 0 )

/**
 * @brief Allocate and initialize a network queue.
 *
 * This function allocates memory for a network queue structure, initializes
 * its members, creates a FreeRTOS queue, and adds the queue to the network
 * queue list.
 *
 * @return A pointer to the allocated network queue structure.
 */
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

/**
 * @brief Create a network queue.
 *
 * This function creates a network queue by calling pxNetworkQueueMalloc(),
 * sets the queue's policy, IP version, name, and filter function.
 *
 * @param ePolicy The queue's policy.
 * @param uxIPV The queue's IP version.
 * @param cName The queue's name.
 * @param fnFilter The queue's filter function.
 * @return A pointer to the created network queue.
 */
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

/**
 * @brief Free a network queue.
 *
 * This function deletes the FreeRTOS queue associated with the network queue
 * and frees the memory allocated for the network queue structure.
 *
 * @param pxQueue A pointer to the network queue to be freed.
 */
void vNetworkQueueFree( NetworkQueue_t * pxQueue )
{
	vQueueDelete( pxQueue->xQueue );
	vPortFree( pxQueue );
}

NetworkQueueItem_t * pxNetworkQueueItemMalloc( )
{
	return pvPortMalloc( sizeof( NetworkQueueItem_t ) );
}

void NetworkQueueItemFree( NetworkQueueItem_t * pxItem )
{
	vPortFree( pxItem );
}

#endif

/**
 * @brief Get the number of packets waiting in a network queue.
 *
 * This function returns the number of packets waiting in a network queue.
 *
 * @param pxQueue A pointer to the network queue.
 * @return The number of packets waiting in the network queue.
 */
UBaseType_t uxNetworkQueuePacketsWaiting( NetworkQueue_t * pxQueue )
{
	return uxQueueMessagesWaiting( pxQueue->xQueue );
}

/**
 * @brief Check if a network queue is empty.
 *
 * This function checks if a network queue is empty by checking if the number
 * of packets waiting in the queue is zero.
 *
 * @param pxQueue A pointer to the network queue.
 * @return pdTRUE if the network queue is empty, pdFALSE otherwise.
 */
BaseType_t xNetworkQueueIsEmpty( NetworkQueue_t * pxQueue )
{
	return uxQueueMessagesWaiting( pxQueue->xQueue ) == 0 ? pdTRUE : pdFALSE;
}
