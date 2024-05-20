
#include <string.h>

#include "FreeRTOS.h"
#include "queue.h"

#include "FreeRTOS_IP.h"

#include "FreeRTOS_TSN_NetworkQueues.h"
#include "FreeRTOS_TSN_NetworkScheduler.h"

NetworkQueueNode_t *pxNetworkQueueRoot = NULL;
NetworkQueueList_t *pxNetworkQueueList = NULL;
UBaseType_t uxNumQueues = 0;
volatile TickType_t uxNextWakeup = 0;

void prvNetworkQueueListAdd( NetworkQueueList_t *pxItem )
{	
	uxNumQueues += 1;

	if( pxNetworkQueueList == NULL )
	{
		pxNetworkQueueList = pxItem;
	}
	else
	{
		NetworkQueueList_t *pxIndex = pxNetworkQueueList;
		while( pxIndex->pxNext != NULL)
		{
			pxIndex = pxIndex->pxNext;
		}
		pxIndex->pxNext = pxItem;
	}
}

#if ( configSUPPORT_DYNAMIC_ALLOCATION != 0 )

NetworkQueueNode_t * pxNetworkQueueNodeCreate( BaseType_t xNumChildren )
{
	NetworkQueueNode_t *pxNode;
	UBaseType_t uxSpaceRequired = sizeof( NetworkQueueNode_t ) + xNumChildren * sizeof( NetworkQueueNode_t* );

	pxNode = pvPortMalloc( uxSpaceRequired );
	
	if( pxNode != NULL )
	{
		memset( pxNode, '\0', uxSpaceRequired );
		pxNode->ucNumChildren = xNumChildren;
	}

	return pxNode;
}

void vNetworkQueueNodeRelease( NetworkQueueNode_t *pxNode )
{
	vPortFree( pxNode );
}

NetworkQueue_t* pxNetworkQueueCreate()
{
	NetworkQueue_t *pxQueue = pvPortMalloc( sizeof( NetworkQueue_t ) );
	NetworkQueueList_t *pxNode;

	if( pxQueue != NULL )
	{
		memset( pxQueue, '\0', sizeof( NetworkQueue_t ) );
		pxQueue->xQueue = xQueueCreate( ipconfigEVENT_QUEUE_LENGTH, sizeof( IPStackEvent_t ) );
		configASSERT( pxQueue->xQueue != NULL );
		pxQueue->xTimeout = netqueueDEFAULT_QUEUE_TIMEOUT;
		
		pxNode = pvPortMalloc( sizeof( NetworkQueueList_t ) );
		pxNode->pxQueue = pxQueue;
		pxNode->pxNext = NULL;

		prvNetworkQueueListAdd( pxNode );
	}

	return pxQueue;
}

NetworkQueue_t* pxNetworkQueueFromIPEventQueue()
{
	NetworkQueue_t *pxQueue = pvPortMalloc( sizeof( NetworkQueue_t ) );
	NetworkQueueList_t *pxNode;

	if( pxQueue != NULL )
	{
		memset( pxQueue, '\0', sizeof( NetworkQueue_t ) );
		pxQueue->xQueue = xNetworkEventQueue;
		pxQueue->xTimeout = netqueueDEFAULT_QUEUE_TIMEOUT;
		
		pxNode = pvPortMalloc( sizeof( NetworkQueueList_t ) );
		pxNode->pxQueue = pxQueue;
		pxNode->pxNext = NULL;

		prvNetworkQueueListAdd( pxNode );
	}

	return pxQueue;
}

void vNetworkQueueRelease( NetworkQueue_t *pxQueue )
{
	vQueueDelete( pxQueue->xQueue );
	vPortFree( pxQueue );
}

#endif

BaseType_t xNetworkQueueAssignRoot( NetworkQueueNode_t *pxNode )
{
	if( pxNetworkQueueRoot == NULL )
	{
		pxNetworkQueueRoot = pxNode;
		return pdPASS;
	}
	else
	{
		return pdFAIL;
	}
}

NetworkQueue_t * pxNetworkQueueSearchMatch( NetworkBufferDescriptor_t *pxNetworkBuffer )
{
	NetworkQueueNode_t *pxIndex;
	BaseType_t xIteration = 0;
	BaseType_t xTemp; 

	if( pxNetworkQueueRoot != NULL )
	{
		/* the maximum number of iteration for the following
		 * loop is the product of the maximum number of children
		 * of each level of the tree.
		 * An upper bound is 2^L, where L is the number of leaves
		 * of the tree, here the number of queues.
		 */
		while( xIteration < ( 1 << uxNumQueues ) )
		{
			xTemp = xIteration;
			pxIndex = pxNetworkQueueRoot;

			while( pxIndex->pxQueue == NULL )
			{
				pxIndex = pxIndex->pxNext[xTemp % pxIndex->ucNumChildren];

				xTemp /= pxIndex->ucNumChildren;

			}

			if( pxIndex->pxQueue->fnFilter( pxNetworkBuffer ) == pdTRUE )
			{
				return pxIndex->pxQueue;
			}

			++xIteration;

		}
	}
	
	return NULL;
}


BaseType_t xNetworkQueueInsertPacket( NetworkBufferDescriptor_t * pxNetworkBuffer )
{
	NetworkQueueList_t *pxIterator = pxNetworkQueueList;
	IPStackEvent_t xEvent;
	
	while( pxIterator != NULL )
	{
		if( pxIterator->pxQueue->fnFilter( pxNetworkBuffer ) )
		{
			xEvent.eEventType = eNetworkRxEvent;
			xEvent.pvData = ( void * ) pxNetworkBuffer;
			return xNetworkQueuePush(pxIterator->pxQueue, ( void * ) &xEvent);
		}
		
		pxIterator = pxIterator->pxNext;
	}

	return pdFAIL;
}

NetworkBufferDescriptor_t * pxNetworkQueueRetrievePacket( void )
{
	NetworkQueue_t * pxChosenQueue;

	if( pxNetworkQueueRoot == NULL )
	{
		return NULL;
	}
	else
	{
		pxChosenQueue = pxNetworkSchedulerCall( pxNetworkQueueRoot );

		if( pxChosenQueue != NULL )
		{
			return pxNetworkQueuePop( pxChosenQueue );
		}
		else
		{
			return NULL;
		}
	}
}

BaseType_t xNetworkQueuePush( NetworkQueue_t * pxQueue, void * pvObject)
{
    return xQueueSendToBack( pxQueue->xQueue, pvObject, pxQueue->xTimeout);
}

NetworkBufferDescriptor_t * pxNetworkQueuePop( NetworkQueue_t * pxQueue )
{
	IPStackEvent_t pxReturn;

	if( xQueueReceive( pxQueue->xQueue, &pxReturn, 0 ) != pdPASS )
	{
		/* queue empty */
		return NULL;
	}

	return ( NetworkBufferDescriptor_t * ) pxReturn.pvData;
}

void vNetworkQueueAddWakeupEvent( TickType_t uxTime )
{
	TickType_t uxNow = xTaskGetTickCount();

	if( uxTime < uxNextWakeup || uxNextWakeup <= uxNow )
	{
		uxNextWakeup = uxTime;
	}
}

TickType_t uxNetworkQueueGetTicksUntilWakeup( void )
{
	TickType_t uxNow = xTaskGetTickCount();

	if( uxNextWakeup <= uxNow )
	{
		return portMAX_DELAY;
	}
	else
	{
		return uxNextWakeup - uxNow;
	}

}
