
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "FreeRTOS_TSN_NetworkSchedulerBlock.h"

TickType_t uxNextWakeup = 0;

BaseType_t prvAlwaysReady( NetworkNode_t * pxNode )
{
	return pdTRUE;
}

NetworkQueue_t * prvSelectFirst( NetworkNode_t * pxNode )
{
	return pxNetworkSchedulerCall( pxNode->pxNext[0] );
}

#if ( configSUPPORT_DYNAMIC_ALLOCATION != 0 )

NetworkNode_t * pxNetworkNodeCreate( BaseType_t xNumChildren )
{
	NetworkNode_t *pxNode;
	UBaseType_t uxSpaceRequired = sizeof( NetworkNode_t ) + xNumChildren * sizeof( NetworkNode_t* );

	pxNode = pvPortMalloc( uxSpaceRequired );
	
	if( pxNode != NULL )
	{
		memset( pxNode, '\0', uxSpaceRequired );
		pxNode->ucNumChildren = xNumChildren;
	}

	return pxNode;
}

void vNetworkNodeRelease( NetworkNode_t *pxNode )
{
	vPortFree( pxNode );
}

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

void vNetworkSchedulerGenericRelease( void * pvSched )
{
	struct xSCHEDULER_GENERIC * pxSched = ( struct xSCHEDULER_GENERIC * ) pvSched;
	pxSched->pxOwner->pvScheduler = NULL;
	vPortFree( pvSched );
}

#endif

NetworkQueue_t * pxNetworkSchedulerCall( NetworkNode_t * pxNode )
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

NetworkBufferDescriptor_t * pxPeekNextPacket( NetworkNode_t * pxNode)
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

void vNetworkQueueAddWakeupEvent( TickType_t uxTime )
{
	TickType_t uxNow = xTaskGetTickCount();

	if( uxTime < uxNextWakeup || uxNextWakeup <= uxNow )
	{
		uxNextWakeup = uxTime;
	}
}



