
#include "FreeRTOS.h"

#include "FreeRTOS_IP.h"
#include "FreeRTOS_IP_Private.h"

#include "FreeRTOS_TSN_Controller.h"
#include "FreeRTOS_TSN_NetworkQueues.h"

static TaskHandle_t xTSNControllerHandle;

BaseType_t xSendEventStructToTSNController( const IPStackEvent_t * pxEvent,
                                     TickType_t uxTimeout )
{
	BaseType_t xReturn = pdFALSE;

    if( pxEvent->eEventType == eNetworkRxEvent )
	{
		xReturn = xNetworkQueueInsertPacket( ( NetworkBufferDescriptor_t * ) pxEvent->pvData );
	}
	else
	{
		xReturn = xSendEventStructToIPTask( pxEvent, uxTimeout );
	}

	return xReturn;
}


static void prvTSNController( void * pvParameters )
{
    NetworkBufferDescriptor_t * pxBuf;

    while( pdTRUE )
    {
        ulTaskNotifyTake( pdTRUE, portMAX_DELAY );

        while( pdTRUE )
        {
            pxBuf = pxNetworkQueueRetrievePacket();

            if( pxBuf == NULL)
            {
                break;
            }
            else
            {
                FreeRTOS_debug_printf( ("Received: %32s\n", pxBuf->pucEthernetBuffer) );
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

BaseType_t xSendPacket( NetworkBufferDescriptor_t * pxBuf )
{
    if( xNetworkQueueInsertPacket( pxBuf ) == pdTRUE)
    {
        xTaskNotifyGive( xTSNControllerHandle );
        return pdTRUE;
    }
    else
    {
        return pdFALSE;
    }
}
