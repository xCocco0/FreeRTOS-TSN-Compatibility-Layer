
#include "FreeRTOS.h"

#include "FreeRTOS_IP.h"
#include "FreeRTOS_IP_Private.h"

#include "FreeRTOS_TSN_Controller.h"
#include "FreeRTOS_TSN_NetworkScheduler.h"
#include "NetworkWrapper.h"

#if ( tsnconfigUSE_PRIO_INHERIT != tsnconfigDISABLE )
	#define controllerTSN_TASK_BASE_PRIO ( tskIDLE_PRIORITY + 1 )
#else
	#define controllerTSN_TASK_BASE_PRIO ( configMAX_PRIORITIES - 1 )
#endif


static TaskHandle_t xTSNControllerHandle;

extern NetworkQueueList_t * pxNetworkQueueList;

BaseType_t xSendEventStructToTSNController( const IPStackEvent_t * pxEvent,
                                     TickType_t uxTimeout )
{
	BaseType_t xReturn = pdFALSE;
	NetworkQueueItem_t xItem;

    if( pxEvent->eEventType == eNetworkRxEvent )
	{
		xItem.eEventType = pxEvent->eEventType;
		xItem.pvData = pxEvent->pvData;
		xItem.xReleaseAfterSend = pdTRUE;

		xReturn = xNetworkQueueInsertPacketByFilter( &xItem );
	}
	else
	{
		xReturn = xSendEventStructToIPTask( pxEvent, uxTimeout );
	}

	return xReturn;
}


static void prvTSNController( void * pvParameters )
{
	NetworkQueueItem_t xItem;
	IPStackEvent_t xEvent;
	NetworkBufferDescriptor_t * pxBuf;
	NetworkQueue_t * pxQueue;
	TickType_t uxTimeToSleep;

    while( pdTRUE )
    {
		uxTimeToSleep = configMIN( uxNetworkQueueGetTicksUntilWakeup(), pdMS_TO_TICKS( tsnconfigCONTROLLER_MAX_EVENT_WAIT) );
		configPRINTF( ("[%lu] Sleeping for %lu ms\r\n", xTaskGetTickCount(), uxTimeToSleep ) );

        ulTaskNotifyTake( pdTRUE, uxTimeToSleep );

        while( pdTRUE )
        {
            pxQueue = xNetworkQueueSchedule();

            if( pxQueue == NULL )
            {
                break;
            }

			if( xNetworkQueuePop( pxQueue, &xItem ) != pdFAIL )
			{
				#if ( tsnconfigUSE_PRIO_INHERIT != tsnconfigDISABLE )

				if( xNetworkQueueIsEmpty( pxQueue ) )
				{
					vTSNControllerComputePriority();
				}

				#endif

				pxBuf = ( NetworkBufferDescriptor_t * ) xItem.pvData;

				/* for debugging */
				if( xItem.eEventType == eNetworkTxEvent )
				{
					if( *( uint16_t * ) &pxBuf->pucEthernetBuffer[ 12 ] == FreeRTOS_htons(0x88a8) )
					{
						
						FreeRTOS_debug_printf( ("Gotcha\r\n") );
					}

					FreeRTOS_debug_printf( ("[%lu]Sending: %32s\n", xTaskGetTickCount(), pxBuf->pucEthernetBuffer) );
					xMAC_NetworkInterfaceOutput( pxBuf->pxInterface, pxBuf, xItem.xReleaseAfterSend );
				}
				else
				{
					/* Receive of other events */
					if( pxQueue->ePolicy == eIPTaskEvents )
					{
						xEvent.eEventType = xItem.eEventType;
						xEvent.pvData = xItem.pvData;
						FreeRTOS_debug_printf( ("[%lu]Forwarding to IP task: %32s\n", xTaskGetTickCount(), pxBuf->pucEthernetBuffer) );
						xSendEventStructToIPTask( &xEvent, pxQueue->uxTimeout );
					}
					else
					{
						FreeRTOS_debug_printf( ("[%lu]Received: %32s\n", xTaskGetTickCount(), pxBuf->pucEthernetBuffer) );
						/*TODO: deliver to tasks */
					}
				}
			}
        }
    }
}

void prvTSNController_Initialise( void )
{
    xTaskCreate( prvTSNController,
                        "TSN-controller",
                        ipconfigIP_TASK_STACK_SIZE_WORDS,
                        NULL,
                        controllerTSN_TASK_BASE_PRIO,
                        &( xTSNControllerHandle ) );
}

BaseType_t xNotifyController()
{
	return xTaskNotifyGive( xTSNControllerHandle );
}

void vTSNControllerComputePriority( void )
{
	NetworkQueueList_t * pxIter = pxNetworkQueueList;
	UBaseType_t uxPriority = controllerTSN_TASK_BASE_PRIO;

	while( pxIter != NULL )
	{
		if( ! xNetworkQueueIsEmpty( pxIter->pxQueue ) )
		{
			if( pxIter->pxQueue->uxIPV > uxPriority )
			{
				uxPriority = pxIter->pxQueue->uxIPV;
			}
		}
		pxIter = pxIter->pxNext;
	}
	
	vTaskPrioritySet( xTSNControllerHandle, uxPriority );
}

BaseType_t xTSNControllerUpdatePriority( UBaseType_t uxPriority )
{
	if( uxTaskPriorityGet( xTSNControllerHandle ) < uxPriority )
	{
		vTaskPrioritySet( xTSNControllerHandle, uxPriority );
		return pdTRUE;
	}
	
	return pdFALSE;
}
