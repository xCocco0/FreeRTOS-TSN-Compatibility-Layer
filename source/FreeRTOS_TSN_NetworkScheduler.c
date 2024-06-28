/**
 * @file FreeRTOS_TSN_NetworkScheduler.c
 * @brief This file contains the implementation of the network scheduler for FreeRTOS TSN Compatibility Layer.
 */

#include "FreeRTOS_TSN_NetworkScheduler.h"
#include "FreeRTOS_TSN_Controller.h"

NetworkNode_t * pxNetworkQueueRoot = NULL;
NetworkQueueList_t * pxNetworkQueueList = NULL;
UBaseType_t uxNumQueues = 0;

/**
 * @brief Matches the filtering policy of a network queue with a network queue item.
 *
 * This function compares the filtering policy of a network queue with a network queue item
 * to determine if they match. The matching is based on the event type of the network queue item.
 *
 * @param pxItem The network queue item to match.
 * @param pxQueue The network queue to match against.
 * @return pdTRUE if the network queue item matches the filtering policy of the network queue, pdFALSE otherwise.
 */
BaseType_t prvMatchQueuePolicy( const NetworkQueueItem_t * pxItem,
                                NetworkQueue_t * pxQueue )
{
    switch( pxQueue->ePolicy )
    {
        case eSendRecv:
            // Match if the event type is either eNetworkTxEvent or eNetworkRxEvent
            return ( pxItem->eEventType == eNetworkTxEvent ) || ( pxItem->eEventType == eNetworkRxEvent );

        case eSendOnly:
            // Match if the event type is eNetworkTxEvent
            return( pxItem->eEventType == eNetworkTxEvent );

        case eRecvOnly:
            // Match if the event type is eNetworkRxEvent
            return( pxItem->eEventType == eNetworkRxEvent );

        case eIPTaskEvents:
            // Match for all event types
            return pdTRUE;

        default:
            // No match for unknown policy
            return pdFALSE;
    }
}

/**
 * @brief Adds a network queue to the network queue list.
 *
 * This function is used to add a network queue to the network queue list.
 * The network queue list is a linked list that keeps track of all the network queues.
 *
 * @param pxItem The network queue to add.
 */
void vNetworkQueueListAdd( NetworkQueueList_t * pxItem )
{
    // Increment the number of queues
    uxNumQueues += 1;

    // If the network queue list is empty, set the new item as the head
    if( pxNetworkQueueList == NULL )
    {
        pxNetworkQueueList = pxItem;
    }
    else
    {
        // Traverse the network queue list to find the last item
        NetworkQueueList_t * pxIndex = pxNetworkQueueList;

        while( pxIndex->pxNext != NULL )
        {
            pxIndex = pxIndex->pxNext;
        }

        // Add the new item to the end of the list
        pxIndex->pxNext = pxItem;
    }
}

/**
 * @brief Assigns the root network node.
 *
 * This function assigns the specified network node as the root node for the TSN controller's scheduling function.
 * The root node is the starting point for the scheduling algorithm.
 *
 * @param pxNode The network node to assign as the root.
 * @return pdPASS if the root network node is successfully assigned, pdFAIL otherwise.
 */
BaseType_t xNetworkQueueAssignRoot( NetworkNode_t * pxNode )
{
    // Check if the root network node is already assigned
    if( pxNetworkQueueRoot == NULL )
    {
        // Assign the specified network node as the root
        pxNetworkQueueRoot = pxNode;
        return pdPASS;
    }
    else
    {
        // Root network node is already assigned, return failure
        return pdFAIL;
    }
}


/**
 * @brief Iterate over the list of network queues and find a match based on the queues' filtering policy.
 *        If more than one queue matches the filter, the one with the highest IPV (Internet Protocol Version) is chosen.
 *
 * @param pxItem The network queue item to insert.
 * @param uxTimeout The timeout value for the insertion operation.
 * @return pdPASS if the network queue item is successfully inserted, pdFAIL otherwise.
 */
BaseType_t xNetworkQueueInsertPacketByFilter(const NetworkQueueItem_t *pxItem, UBaseType_t uxTimeout)
{
    NetworkQueueList_t *pxIterator = pxNetworkQueueList; // Iterator for network queue list
    NetworkBufferDescriptor_t *pxNetworkBuffer = (NetworkBufferDescriptor_t *)pxItem->pxBuf; // Network buffer descriptor
    NetworkQueue_t *pxChosenQueue = NULL; // Chosen network queue

    while (pxIterator != NULL)
    {
        // Check if the network buffer matches the filtering policy and the queue policy
        if (pxIterator->pxQueue->fnFilter(pxNetworkBuffer) && prvMatchQueuePolicy(pxItem, pxIterator->pxQueue))
        {
            if (pxChosenQueue != NULL)
            {
                // If a chosen queue already exists, compare the IPV values and choose the one with the highest value
                if (pxIterator->pxQueue->uxIPV <= pxChosenQueue->uxIPV)
                {
                    pxIterator = pxIterator->pxNext;
                    continue;
                }
            }

            pxChosenQueue = pxIterator->pxQueue; // Set the chosen queue
        }

        pxIterator = pxIterator->pxNext; // Move to the next network queue
    }

    if (pxChosenQueue != NULL)
    {
        return xNetworkQueuePush(pxChosenQueue, pxItem, uxTimeout); // Push the network queue item to the chosen queue
    }
    else
    {
        return pdFAIL; // No matching queue found, return failure
    }
}

/**
 * @brief Inserts a network queue item into a network queue based on the queue name.
 *
 * This function inserts a network queue item into a network queue identified by its name.
 * The network queue item is inserted using the xNetworkQueuePush() function.
 *
 * @param pxItem The network queue item to insert.
 * @param pcQueueName The name of the network queue to insert into.
 * @param uxTimeout The timeout value for the insertion operation.
 * @return pdPASS if the network queue item is successfully inserted, pdFAIL otherwise.
 */
BaseType_t xNetworkQueueInsertPacketByName( const NetworkQueueItem_t * pxItem,
                                            char * pcQueueName,
                                            UBaseType_t uxTimeout )
{
    NetworkQueue_t * pxQueue;

    // Find the network queue by name
    pxQueue = pxNetworkQueueFindByName( pcQueueName, pxItem );

    if( pxQueue != NULL )
    {
        // Insert the network queue item into the found queue
        return xNetworkQueuePush( pxQueue, pxItem, uxTimeout );
    }

    return pdFALSE;
}

/**
 * @brief Schedules the network queues and returns the chosen network queue.
 *
 * This function is responsible for scheduling the network queues and selecting the next
 * network queue to be processed. It checks if there is a network queue available and
 * calls the network scheduler function to make the selection. If there is no network
 * queue available, it returns pdFAIL.
 *
 * @return The chosen network queue, or pdFAIL if no network queue is available.
 */
NetworkQueue_t * xNetworkQueueSchedule( void )
{
    // Check if there is a network queue available
    if( pxNetworkQueueRoot != NULL )
    {
        // Call the network scheduler function to select the next network queue
        return pxNetworkSchedulerCall( pxNetworkQueueRoot );
    }

    // No network queue available
    return pdFAIL;
}

/**
 * @brief Pushes a network queue item into a network queue.
 *
 * This function pushes a network queue item into a network queue. It first calls the
 * `fnOnPush` callback function if queue event callbacks are enabled. Then, it uses
 * the `xQueueSendToBack` function to send the item to the back of the queue. If the
 * item is successfully pushed, it updates the priority of the TSN controller (if
 * dynamic priority is enabled), notifies the controller, and returns `pdPASS`.
 * Otherwise, it returns `pdFAIL`.
 *
 * @param pxQueue The network queue to push the item into.
 * @param pxItem The network queue item to push.
 * @param uxTimeout The timeout value for the push operation.
 * @return pdPASS if the network queue item is successfully pushed, pdFAIL otherwise.
 */
BaseType_t xNetworkQueuePush( NetworkQueue_t * pxQueue,
                              const NetworkQueueItem_t * pxItem,
                              UBaseType_t uxTimeout )
{
    // Call the fnOnPush callback function if queue event callbacks are enabled
    #if ( tsnconfigINCLUDE_QUEUE_EVENT_CALLBACKS != tsnconfigDISABLE )
        pxQueue->fnOnPush( ( NetworkBufferDescriptor_t * ) pxItem->pvData );
    #endif

    // Send the item to the back of the queue using xQueueSendToBack function
    if( xQueueSendToBack( pxQueue->xQueue, ( void * ) pxItem, uxTimeout ) == pdPASS )
    {
        // Update the priority of the TSN controller if dynamic priority is enabled
        #if ( tsnconfigCONTROLLER_HAS_DYNAMIC_PRIO != tsnconfigDISABLE )
            xTSNControllerUpdatePriority( pxQueue->uxIPV );
        #endif

        // Notify the controller
        xNotifyController();

        // Return pdPASS to indicate successful push
        return pdPASS;
    }

    // Return pdFAIL to indicate failed push
    return pdFAIL;
}

/**
 * @brief Pops a network queue item from a network queue.
 *
 * This function pops an item from the specified network queue. If the queue is empty,
 * the function will wait for a specified timeout period for an item to become available.
 *
 * @param pxQueue The network queue to pop the item from.
 * @param pxItem The network queue item to pop.
 * @param uxTimeout The timeout value for the pop operation.
 * @return pdPASS if a network queue item is successfully popped, pdFAIL otherwise.
 */
BaseType_t xNetworkQueuePop( NetworkQueue_t * pxQueue,
                             NetworkQueueItem_t * pxItem,
                             UBaseType_t uxTimeout )
{
    // Check if the queue is empty
    if( xQueueReceive( pxQueue->xQueue, pxItem, uxTimeout ) != pdPASS )
    {
        /* queue empty */
        return pdFAIL;
    }

    // Call the callback function if enabled
    #if ( tsnconfigINCLUDE_QUEUE_EVENT_CALLBACKS != tsnconfigDISABLE )
        pxQueue->fnOnPop( ( NetworkBufferDescriptor_t * ) pxItem->pvData );
    #endif

    return pdPASS;
}

#if ( tsnconfigMAX_QUEUE_NAME_LEN != 0 )

/**
 * @brief Finds a network queue by its name.
 *
 * This function searches for a network queue with a specific name in the network queue list.
 * It matches the filtering policy specified by the `pxItem` parameter.
 *
 * @param pcName The name of the network queue to find.
 * @param pxItem The network queue item to match the filtering policy.
 * @return The network queue if found, NULL otherwise.
 */
NetworkQueue_t * pxNetworkQueueFindByName( char * pcName,
                                           const NetworkQueueItem_t * pxItem )
{
    NetworkQueueList_t * pxIterator = pxNetworkQueueList;

    // Iterate through the network queue list
    while( pxIterator != NULL )
    {
        // Check if the filtering policy matches
        if( prvMatchQueuePolicy( pxItem, pxIterator->pxQueue ) )
        {
            // Check if the names match
            if( strncmp( pcName, pxIterator->pxQueue->cName, tsnconfigMAX_QUEUE_NAME_LEN ) == 0 )
            {
                // Return the network queue if found
                return pxIterator->pxQueue;
            }
        }

        // Move to the next network queue in the list
        pxIterator = pxIterator->pxNext;
    }

    // Return NULL if the network queue is not found
    return NULL;
}

#endif /* if ( tsnconfigMAX_QUEUE_NAME_LEN != 0 ) */
