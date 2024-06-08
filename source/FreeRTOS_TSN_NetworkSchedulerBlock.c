
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

BaseType_t xNetworkSchedulerLinkQueue( NetworkNode_t * pxNode, NetworkQueue_t * pxQueue )
{
	if( pxNode != NULL )
	{
		pxNode->pxQueue = pxQueue;
		return pdPASS;
	}

	return pdFAIL;
}

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



