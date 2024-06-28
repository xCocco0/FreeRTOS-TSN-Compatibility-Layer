/**
 * @file NetworkQueueExample.c
 * @brief An example of network queue hierarchy configuration
 *
 * This file contains an example setup for the configuration of the network
 * queues. This should be in the project folder and is part of the user
 * project.
 * The user should define vNetworkQueueInit() to allocate and link the
 * network queues; this should always end with a call to
 * xNetworkQueueAssignRoot().
 * Remember the following rules:
 * - The number of children of a scheduler is defined when it is allocated
 * - A scheduler should have all of its children correctly linked
 * - A Scheduler should always end either pointing to a queue (leaf) or
 *   to other schedulers (nodes), as many as required by the scheduler type
 */

#include <string.h>

#include "FreeRTOS_TSN_NetworkScheduler.h"

#include "BasicSchedulers.h"
#include "SchedCBS.h"

/** @brief Filter function
 * Keep the signature as is. The return type is either pdTRUE if the packet
 * is to be allowed, or pdFALSE, otherwise.
 */
BaseType_t Port10001( NetworkBufferDescriptor_t * pxNetworkBuffer )
{
    return ( ( UDPPacket_t * ) pxNetworkBuffer->pucEthernetBuffer )->xUDPHeader.usDestinationPort == FreeRTOS_htons( 10001 ) ? pdTRUE : pdFALSE ;
}

BaseType_t Port10003( NetworkBufferDescriptor_t * pxNetworkBuffer )
{
    return ( ( UDPPacket_t * ) pxNetworkBuffer->pucEthernetBuffer )->xUDPHeader.usDestinationPort == FreeRTOS_htons( 10003 ) ? pdTRUE : pdFALSE;
}

/** @brief Function to initialise the queue hierarchy
 * This will be called by the TSN Network Wrapper and should always end with
 * a call to xNetworkQueueAssignRoot()
 */
void vNetworkQueueInit( void )
{
    NetworkQueue_t * queue1, * queue2_r, * queue2_s, * queue3;

    queue1 = pxNetworkQueueCreate( eSendRecv, configMAX_PRIORITIES - 1, "queue1", Port10001 );
    queue2_s = pxNetworkQueueCreate( eSendOnly, 1, "queue2_s", Port10003 );
    queue2_r = pxNetworkQueueCreate( eRecvOnly, 1, "queue2_r", Port10003 );
    queue3 = pxNetworkQueueCreate( eIPTaskEvents, 0, "queue3", NULL );

	/** Schema of the desired layout
	 *
	 *              +-0---------------->[fifoHP]->queue1
	 *              |
	 *              |               +-0->[cbs]->queue2_s
	 * root->[prio]-+-1->[prio_cbs]-|
	 *              |               +-1->[fifo2]->queue2_r
	 *              |
	 *              +-2---------------->[fifo]->queue3
	 */

    NetworkNode_t * prio, * fifo, * fifo2, * cbs, * fifoHP, * prio_cbs;

    fifoHP = pxNetworkNodeCreateFIFO();
    ( void ) xNetworkSchedulerLinkQueue( fifoHP, queue1 );

    cbs = pxNetworkNodeCreateCBS( 57 * 8, 57 * 8 * 2 );
    ( void ) xNetworkSchedulerLinkQueue( cbs, queue2_s );

    fifo2 = pxNetworkNodeCreateFIFO();
    ( void ) xNetworkSchedulerLinkQueue( fifo2, queue2_r );

    fifo = pxNetworkNodeCreateFIFO();
    ( void ) xNetworkSchedulerLinkQueue( fifo, queue3 );

	prio_cbs = pxNetworkNodeCreatePrio( 2 );
    ( void ) xNetworkSchedulerLinkChild( prio_cbs, cbs, 0 );
    ( void ) xNetworkSchedulerLinkChild( prio_cbs, fifo2, 1 );

    prio = pxNetworkNodeCreatePrio( 3 );
    ( void ) xNetworkSchedulerLinkChild( prio, fifoHP, 0 );
    ( void ) xNetworkSchedulerLinkChild( prio, prio_cbs, 1 );
    ( void ) xNetworkSchedulerLinkChild( prio, fifo, 2 );

    ( void ) xNetworkQueueAssignRoot( prio );
}
