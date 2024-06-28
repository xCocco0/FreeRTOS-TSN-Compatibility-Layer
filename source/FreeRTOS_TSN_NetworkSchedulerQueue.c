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
 * It simply returns pdPASS without performing any action.
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
    NetworkQueue_t * pxQueue = pvPortMalloc( sizeof( NetworkQueue_t ) ); // Allocate memory for the network queue structure
    NetworkQueueList_t * pxNode;

    if( pxQueue != NULL )
    {
        memset( pxQueue, '\0', sizeof( NetworkQueue_t ) ); // Initialize the network queue structure with zeros
        pxQueue->xQueue = xQueueCreate( ipconfigEVENT_QUEUE_LENGTH, sizeof( NetworkQueueItem_t ) ); // Create a FreeRTOS queue
        configASSERT( pxQueue->xQueue != NULL ); // Check if the queue was created successfully
        pxQueue->ePolicy = eSendRecv; // Set the queue policy to eSendRecv
        pxQueue->uxIPV = 0; // Set the queue IPV value to 0

        #if ( tsnconfigINCLUDE_QUEUE_EVENT_CALLBACKS != tsnconfigDISABLE )
            pxQueue->fnOnPop = prvDefaultPacketHandler; // Set the callback function for packet pop event
            pxQueue->fnOnPush = prvDefaultPacketHandler; // Set the callback function for packet push event
        #endif

        pxNode = pvPortMalloc( sizeof( NetworkQueueList_t ) ); // Allocate memory for the network queue list structure
        pxNode->pxQueue = pxQueue; // Set the network queue in the list structure
        pxNode->pxNext = NULL; // Set the next pointer to NULL

        vNetworkQueueListAdd( pxNode ); // Add the network queue to the network queue list
    }

    return pxQueue; // Return the allocated network queue structure
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
NetworkQueue_t * pxNetworkQueueCreate( eQueuePolicy_t ePolicy,
                                       UBaseType_t uxIPV,
                                       char * cName,
                                       FilterFunction_t fnFilter )
{
    // Allocate memory for the network queue
    NetworkQueue_t * pxQueue = pxNetworkQueueMalloc();

    // Set the queue's name
    if( cName != NULL )
    {
        strcpy( ( char * ) &pxQueue->cName, cName );
    }
    else
    {
        pxQueue->cName[ 0 ] = '\0';
    }

    // Set the queue's filter function
    if( fnFilter != NULL )
    {
        pxQueue->fnFilter = fnFilter;
    }
    else
    {
        pxQueue->fnFilter = prvAlwaysTrue;
    }

    // Set the queue's policy and IP version
    pxQueue->ePolicy = ePolicy;
    pxQueue->uxIPV = uxIPV;

    return pxQueue;
}

/**
 * @file FreeRTOS_TSN_NetworkSchedulerQueue.c
 * @brief Implementation of network queue functions.
 */

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
    // Delete the FreeRTOS queue associated with the network queue
    vQueueDelete( pxQueue->xQueue );

    // Free the memory allocated for the network queue structure
    vPortFree( pxQueue );
}

/**
 * @brief Allocate memory for a network queue item.
 *
 * This function allocates memory for a network queue item of type NetworkQueueItem_t.
 *
 * @return A pointer to the allocated network queue item.
 */
NetworkQueueItem_t * pxNetworkQueueItemMalloc()
{
    return pvPortMalloc( sizeof( NetworkQueueItem_t ) );
}

/**
 * @brief Free a network queue item.
 *
 * This function frees the memory allocated for a network queue item.
 *
 * @param pxItem A pointer to the network queue item to be freed.
 */
void NetworkQueueItemFree( NetworkQueueItem_t * pxItem )
{
    // Free the memory allocated for the network queue item
    vPortFree( pxItem );
}

#endif /* if ( configSUPPORT_DYNAMIC_ALLOCATION != 0 ) */

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
