
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

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

NetworkBufferDescriptor_t * pxPeekNextPacket( NetworkQueueNode_t * pxNode)
{
    IPStackEvent_t xEvent;

    if( xQueuePeek( pxNode->pxQueue->xQueue, &xEvent, 0) == pdTRUE )
    {
        return ( NetworkBufferDescriptor_t * ) xEvent.pvData;
    }
    else
    {
        return NULL;
    }
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
	UBaseType_t uxBandwidth;                /*< bandwidth in bits per second */
    UBaseType_t uxMaxCredit;                /*< max credit in bits
                                                regulates burstiness */
	TickType_t uxNextActivation;
};


BaseType_t prvCBSReady( NetworkQueueNode_t * pxNode )
{
    struct xSCHEDULER_CBS * pxSched = ( struct xSCHEDULER_CBS * ) pxNode->pvScheduler;
    TickType_t uxDelay, uxNow, uxMaxDelay;
    NetworkBufferDescriptor_t * pxNextPacket;

    uxNow = xTaskGetTickCount();

    if( pxSched->uxNextActivation <= uxNow )
    {
        pxNextPacket = pxPeekNextPacket( pxNode );
        uxDelay = pdMS_TO_TICKS( ( pxNextPacket->xDataLength * 8 * 1000 ) / ( pxSched->uxBandwidth ) );
        uxMaxDelay = pdMS_TO_TICKS( ( pxSched->uxMaxCredit * 1000 ) / ( pxSched->uxBandwidth ) );

        pxSched->uxNextActivation = configMAX( pxSched->uxNextActivation + uxDelay + uxMaxDelay, uxNow );
        if( pxSched->uxNextActivation > uxMaxDelay)
        {
            pxSched->uxNextActivation -= uxMaxDelay;
        }
        else
        {
            pxSched->uxNextActivation = 0;
        }

        return pdTRUE;
    }
    else
    {
        return pdFALSE;
    }
}

NetworkQueueNode_t * pxNetworkQueueNodeCreateCBS( UBaseType_t uxBandwidth, UBaseType_t uxMaxCredit )
{

	NetworkQueueNode_t *pxNode;
	struct xSCHEDULER_CBS * pxSched;

	pxNode = pxNetworkQueueNodeCreate( 1 );
	pxSched = ( struct xSCHEDULER_CBS * ) pvNetworkSchedulerGenericCreate( pxNode, sizeof( struct xSCHEDULER_CBS ) );
	
	pxSched->uxBandwidth = uxBandwidth;
    pxSched->uxMaxCredit = uxMaxCredit;
    pxSched->xScheduler.fnReady = prvCBSReady;
    pxSched->uxNextActivation = xTaskGetTickCount();

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




