
#include "FreeRTOS.h"

#include "FreeRTOS_IP.h"
#include "FreeRTOS_IP_Private.h"

#include "FreeRTOS_TSN_Controller.h"
#include "FreeRTOS_TSN_NetworkScheduler.h"
#include "NetworkWrapper.h"

static TaskHandle_t xTSNControllerHandle;

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
		uxTimeToSleep = configMIN( uxNetworkQueueGetTicksUntilWakeup(), controllerMAX_EVENT_WAIT_TIME );
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
				pxBuf = ( NetworkBufferDescriptor_t * ) xItem.pvData;

				/* for debugging */
				if( xItem.eEventType == eNetworkTxEvent )
				{
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
                        configMAX_PRIORITIES - 1,
                        &( xTSNControllerHandle ) );
}


BaseType_t xNotifyController()
{
	return xTaskNotifyGive( xTSNControllerHandle );
}


