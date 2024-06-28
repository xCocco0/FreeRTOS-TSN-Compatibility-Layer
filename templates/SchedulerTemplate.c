/**
 * @file SchedulerTemplate.c
 * @brief Template for creating new schedulers
 *
 * This file contains a dummy implementation to use as a reference for
 * defining new schedulers.
 */

#include "SchedulerTemplate.h"

/** This structure will contain all the parameters of your custom scheduler.
 * Remeber that the first member should always be of type struct
 * xSCHEDULER_GENERIC. You can add as many parameters as need after this.
 */
struct xSCHEDULER_YOURNAME
{
    struct xSCHEDULER_GENERIC xScheduler;
	/* Your parameters here */
	UBaseType_t uxParam1;
	UBaseType_t uxParam2;
	TickType_t uxNextActivation;
	/* More ... */
};

/** @brief This is the ready function of the scheduler.
 * You can change the name as you wish but keep the parameters and return
 * type as it is. The return type should be pdTRUE if the scheduler is ready,
 * or pdFALSE if the scheduler is not.
 */
BaseType_t prvYourNameReady( NetworkNode_t * pxNode )
{
    struct xSCHEDULER_YOURNAME * pxSched = ( struct xSCHEDULER_YOURNAME * ) pxNode->pvScheduler;

	/* This simple implementation will guarantee that packets are spaced by
	 * uxParam1 ticks. Please note the use of vNetworkQueueAddWakeupEvent().
	 * The default behaviour for the scheduler when there is not packet to
	 * sleep until a new packet is sent/received up to a maximum sleep time.
	 * In order to guarantee correct timing, the implementation should
	 * hint the scheduler on when it should wake up.
	 */
    uxNow = xTaskGetTickCount();

    if( pxSched->uxNextActivation <= uxNow )
    {
		pxSched->uxNextActivation += pxSched->uxParam1;
        vNetworkQueueAddWakeupEvent( pxSched->uxNextActivation );
		return pdTRUE;
    }
    else
    {
        return pdFALSE;
    }
}

/** @brief Function to select the children queue.
 * Please don't change the parameters and return type.
 * This function should return the queue to be scheduled or NULL if no queue
 * can be scheduled. Note that this function only makes sense when and if
 * this scheduler has children schedulers linked.
 */
NetworkQueue_t * prvYourNameSelect( NetworkNode_t * pxNode )
{
    NetworkQueue_t * pxResult = NULL;

    for( uint16_t usIter = 0; usIter < pxNode->ucNumChildren; ++usIter )
    {
		/* You should use pxNetworkSchedulerCall to recursively select
		 * leaf queues. If return NULL if no children queue of the selected
		 * scheduler can be scheduled
		 */
        pxResult = pxNetworkSchedulerCall( pxNode->pxNext[ usIter ] );

        if( pxResult != NULL )
        {
            break;
        }
    }
}

NetworkNode_t * pxNetworkNodeCreateYourName( UBaseType_t uxParam1, UBaseType_t uxParam2 );
{
    NetworkNode_t * pxNode;
    struct xSCHEDULER_CBS * pxSched;

	/* Specify the number of children in pxNetworkNodeCreate(). Please note
	 * that schedulers with more than 1 children cannot have a queue linked.
	 * You can only link other schedulers.
	 */
    pxNode = pxNetworkNodeCreate( uxParam2 );
    pxSched = ( struct xSCHEDULER_YOURNAME * ) pvNetworkSchedulerGenericCreate( pxNode, sizeof( struct xSCHEDULER_YOURNAME ) );

    pxSched->uxParam1 = uxParam1;
    pxSched->uxParam2 = uxParam2;
    pxSched->uxNextActivation = xTaskGetTickCount();
	
	/* If you don't specify the ready function, the default one will be used.
	 * The default function will always return pdTRUE
	 */
    pxSched->xScheduler.fnReady = prvYourNameReady;

	/* If you don't specify the select function, the default one will always
	 * select the first children. If your scheduler has no children schedulers
	 * or no queues linked, this won't be used at all.
	 */
    pxSched->xScheduler.fnSelect = prvYourNameSelect;

    return pxNode;
}
