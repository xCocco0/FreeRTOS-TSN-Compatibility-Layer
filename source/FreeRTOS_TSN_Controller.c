
#include "FreeRTOS.h"

#include "FreeRTOS_IP.h"
#include "FreeRTOS_IP_Private.h"

#include "FreeRTOS_TSN_Controller.h"
#include "FreeRTOS_TSN_NetworkScheduler.h"

static TaskHandle_t xTSNControllerHandle;

BaseType_t xSendEventStructToTSNController( const IPStackEvent_t * pxEvent,
                                     TickType_t uxTimeout )
{
	BaseType_t xReturn = pdFALSE;

    if( pxEvent->eEventType == eNetworkRxEvent )
	{
		xReturn = xNetworkQueueInsertPacket( pxEvent );
	}
	else
	{
		xReturn = xSendEventStructToIPTask( pxEvent, uxTimeout );
	}

	return xReturn;
}


static void prvTSNController( void * pvParameters )
{
    IPStackEvent_t xEvent;
	NetworkBufferDescriptor_t * pxBuf;
	TickType_t uxTimeToSleep;

    while( pdTRUE )
    {
		uxTimeToSleep = configMIN( uxNetworkQueueGetTicksUntilWakeup(), controllerMAX_EVENT_WAIT_TIME );
		configPRINTF( ("[%lu] Sleeping for %lu ms\r\n", xTaskGetTickCount(), uxTimeToSleep ) );

        ulTaskNotifyTake( pdTRUE, uxTimeToSleep);

        while( pdTRUE )
        {
            
            if( xNetworkQueueRetrievePacket( &xEvent ) != pdPASS )
            {
                break;
            }

			pxBuf = ( NetworkBufferDescriptor_t * ) xEvent.pvData;

			/* for debugging */
			if( xEvent.eEventType == eNetworkRxEvent )
			{
				FreeRTOS_debug_printf( ("[%lu]Received: %32s\n", xTaskGetTickCount(), pxBuf->pucEthernetBuffer) );
			}
			else
			{
				FreeRTOS_debug_printf( ("[%lu]Sending: %32s\n", xTaskGetTickCount(), pxBuf->pucEthernetBuffer) );
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


