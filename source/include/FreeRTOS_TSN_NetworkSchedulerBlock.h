#ifndef FREERTOS_TSN_NETWORK_SCHEDULER_BLOCK_H
#define FREERTOS_TSN_NETWORK_SCHEDULER_BLOCK_H

#include "FreeRTOS_TSN_NetworkSchedulerQueue.h"

/** @brief This is the structure the stores the nodes of the network scheduler
 *
 * A generic node has either other nodes as children, specified in the pxNext
 * array which has size ucNumChildren, of a pxQueue, which is a leaf in the
 * network scheduler, and in that case ucNumChildren and pxNext will be
 * ignored. The two cases are distinguished by checking if pxQueue is NULL.
 */
struct xNETQUEUE_NODE
{
    uint8_t ucNumChildren; /**< Number of children nodes in pxNext array. If pxQueue is not NULL this is ignored */
    void * pvScheduler; /**< Pointer to the scheduler which stores the ready and select function pointers */
    struct xNETQUEUE * pxQueue; /**< Pointer to a leaf of the scheduler. If this NULL, than the scheduler will recurse in the children stored in the pxNext field */
    struct xNETQUEUE_NODE * pxNext[]; /**< The array which stores pointer to the children of this node */
};

typedef struct xNETQUEUE_NODE NetworkNode_t;

typedef NetworkQueue_t * ( * SelectQueueFunction_t ) ( NetworkNode_t * pxNode );

typedef BaseType_t ( * ReadyQueueFunction_t ) ( NetworkNode_t * pxNode );

/** @brief A generic structure for implementing a scheduler
 * 
 * This is not indented to be used as is, but should be implemented inside
 * a network scheduler as first member.
 * The size is the length of the specilized version of this struct, which
 * also takes into consideration data iin ucAttributes.
 * The select function should return a pointer to chosen network queue in
 * the entire subtree spanned by the owner, or NULL if not queue can be
 * scheduled.
 * The ready function should return pdTRUE if the queue is allowed to
 * schedule a packet, or pdFALSE if not.
 */
struct xSCHEDULER_GENERIC
{
    uint16_t usSize; /**< The total length of this structure, counting also the flexible members */
    struct xNETQUEUE_NODE * pxOwner; /**< Pointer to the node in which this scheduler is used */
    SelectQueueFunction_t fnSelect; /**< Function to select a children of the owner network node */
    ReadyQueueFunction_t fnReady; /**< Function to determine if the underlining network node is allowed to schedule a packet */
    char ucAttributes[]; /**< This contains the attributes of the different scheduler implementations */
};

#if ( configSUPPORT_DYNAMIC_ALLOCATION != 0 )

    NetworkNode_t * pxNetworkNodeCreate( UBaseType_t uxNumChildren );

    void vNetworkNodeRelease( NetworkNode_t * pxNode );

    void * pvNetworkSchedulerGenericCreate( NetworkNode_t * pxNode,
                                            uint16_t usSize );

    void vNetworkSchedulerGenericRelease( void * pvSched );

#endif /* if ( configSUPPORT_DYNAMIC_ALLOCATION != 0 ) */

BaseType_t xNetworkSchedulerLinkQueue( NetworkNode_t * pxNode,
                                       NetworkQueue_t * pxQueue );

BaseType_t xNetworkSchedulerLinkChild( NetworkNode_t * pxNode,
                                       NetworkNode_t * pxChild,
                                       size_t uxPosition );

NetworkQueue_t * pxNetworkSchedulerCall( NetworkNode_t * pxNode );

NetworkBufferDescriptor_t * pxPeekNextPacket( NetworkNode_t * pxNode );

TickType_t uxNetworkQueueGetTicksUntilWakeup( void );

void vNetworkQueueAddWakeupEvent( TickType_t uxTime );

#define netschedCALL_SELECT_FROM_NODE( pxNode ) \
    ( ( ( struct xSCHEDULER_GENERIC * ) pxNode->pvScheduler )->fnSelect( pxNode ) )

#define netschedCALL_READY_FROM_NODE( pxNode ) \
    ( ( ( struct xSCHEDULER_GENERIC * ) pxNode->pvScheduler )->fnReady( pxNode ) )

#endif /* FREERTOS_TSN_NETWORK_SCHEDULER_H */
