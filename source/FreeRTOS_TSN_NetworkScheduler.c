
#include "FreeRTOS.h"
#include "queue.h"

#include "FreeRTOS_TSN_NetworkScheduler.h"
#include "FreeRTOS_TSN_NetworkQueues.h"

BaseType_t prvAlwaysReady( NetworkQueueNode_t * pxNode )
{
	return pdTRUE;
}

NetworkQueue_t * prvSelectFirst( NetworkQueueNode_t * pxNode )
{
	return pxNetworkSchedulerCall( pxNode->pxNext[0] );
}

void * pvNetworkSchedulerGenericCreate( NetworkQueueNode_t * pxNode, uint16_t usSize )
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

void vNetworkSchedulerGenericRelease( void * pvSched )
{
	struct xSCHEDULER_GENERIC * pxSched = ( struct xSCHEDULER_GENERIC * ) pvSched;
	pxSched->pxOwner->pvScheduler = NULL;
	vPortFree( pvSched );
}

NetworkQueue_t * pxNetworkSchedulerCall( NetworkQueueNode_t * pxNode )
{
	NetworkQueue_t * pxResult = NULL;

	if( netschedCALL_READY_FROM_NODE( pxNode ) == pdTRUE )
	{
		/* scheduler is ready */

		if( pxNode->pxQueue != NULL )
		{
			/* terminal node */
			if( uxQueueMessagesWaiting( pxNode->pxQueue->xQueue ) != 0 )
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

/*----------------------------------------------------------------------------*/

struct xSCHED_RR
{
	struct xSCHEDULER_GENERIC xScheduler;
};

/*----------------------------------------------------------------------------*/

struct xSCHED_PRIO
{
	struct xSCHEDULER_GENERIC xScheduler;
};

NetworkQueue_t * prvPrioritySelect( NetworkQueueNode_t * pxNode )
{
	NetworkQueue_t * pxResult = NULL;

	for( uint16_t usIter = 0; usIter < pxNode->ucNumChildren; ++usIter )
	{
		pxResult = pxNetworkSchedulerCall( pxNode->pxNext[ usIter ] );
		if( pxResult != NULL )
		{
			break;
		}
	}

	return pxResult;
}

/*----------------------------------------------------------------------------*/

struct xSCHEDULER_CBS
{
	struct xSCHEDULER_GENERIC xScheduler;
	UBaseType_t uxBandwidth;
	TickType_t uxNextActivation;
	//TODO
};

NetworkQueueNode_t * pxNetworkQueueNodeCreateCBS( UBaseType_t uxBandwidth )
{

	NetworkQueueNode_t *pxNode;
	struct xSCHEDULER_CBS * pxSched;

	pxNode = pxNetworkQueueNodeCreate( 1 );
	pxSched = ( struct xSCHEDULER_CBS * ) pvNetworkSchedulerGenericCreate( pxNode, sizeof( struct xSCHEDULER_CBS ) );
	
	pxSched->uxBandwidth = uxBandwidth;

	return pxNode;
}

/*----------------------------------------------------------------------------*/

struct xSCHEDULER_FIFO
{
	struct xSCHEDULER_GENERIC xScheduler;
};

NetworkQueueNode_t * pxNetworkQueueNodeCreateFIFO()
{

	NetworkQueueNode_t *pxNode;
	struct xSCHEDULER_FIFO * pxSched;

	pxNode = pxNetworkQueueNodeCreate( 1 );
	pxSched = ( struct xSCHEDULER_FIFO * ) pvNetworkSchedulerGenericCreate( pxNode, sizeof( struct xSCHEDULER_FIFO ) );

	( void ) pxSched;

	return pxNode;
}




