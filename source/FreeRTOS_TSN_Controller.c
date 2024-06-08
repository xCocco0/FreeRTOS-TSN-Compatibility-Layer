
#include "FreeRTOS.h"

#include "FreeRTOS_IP.h"
#include "FreeRTOS_IP_Private.h"

#include "FreeRTOSTSNConfig.h"
#include "FreeRTOSTSNConfigDefaults.h"

#include "FreeRTOS_TSN_Controller.h"
#include "FreeRTOS_TSN_NetworkScheduler.h"
#include "FreeRTOS_TSN_VLANTags.h"
#include "NetworkWrapper.h"

#if ( tsnconfigCONTROLLER_HAS_DYNAMIC_PRIO != tsnconfigDISABLE )
	#define controllerTSN_TASK_BASE_PRIO ( tskIDLE_PRIORITY + 1 )
#else
	#define controllerTSN_TASK_BASE_PRIO ( tsnconfigTSN_CONTROLLER_PRIORITY )
#endif


static TaskHandle_t xTSNControllerHandle;

extern NetworkQueueList_t * pxNetworkQueueList;

void prvDeliverFrame( NetworkBufferDescriptor_t * pxBuf )
{
	uint8_t ucTagsSize;
	EthernetHeader_t * pxEthernetHeader;
	UDPPacket_t * pxUDPPacket;
	BaseType_t xIsWaitingARPResolution;

	if( pxBuf == NULL )
	{
		return;
	}

	if( ( pxBuf->pxInterface == NULL ) || ( pxBuf->pxEndPoint == NULL ) )
	{
		return;
	}

	if( pxBuf->xDataLength < sizeof( EthernetHeader_t ) )
	{
		return;
	}

	ucTagsSize = sizeof( struct xVLAN_TAG ) * ucGetNumberOfTags( pxBuf );

	/* this will remove the vlan tags for reusing the
	 * FreeRTOS plus TCP functions */
	memmove( &pxBuf->pucEthernetBuffer[ vlantagETH_TAG_OFFSET ],
	         &pxBuf->pucEthernetBuffer[ vlantagETH_TAG_OFFSET + ucTagsSize ],
	         pxBuf->xDataLength - vlantagETH_TAG_OFFSET );
	
	pxEthernetHeader = ( EthernetHeader_t * ) pxBuf->pucEthernetBuffer;

	switch( pxEthernetHeader->usFrameType )
	{
		case ipIPv4_FRAME_TYPE:
			/* check checksum
			 * check destination address is my address
			 * check length 
			 * ... */
			if( pxBuf->xDataLength < sizeof( IPPacket_t ) )
			{
				vReleaseNetworkBufferAndDescriptor( pxBuf );
				return;
			}

			pxUDPPacket = ( UDPPacket_t * ) pxBuf->pucEthernetBuffer;
			pxBuf->usPort = pxUDPPacket->xUDPHeader.usSourcePort;
        	pxBuf->xIPAddress.ulIP_IPv4 = pxUDPPacket->xIPHeader.ulSourceIPAddress;

			break;
			
		case ipIPv6_FRAME_TYPE:
			/* IPv6 support TODO */
		case ipARP_FRAME_TYPE:
		default:
			vReleaseNetworkBufferAndDescriptor( pxBuf );
			return;
	}

	if( xProcessReceivedUDPPacket( pxBuf,
						pxUDPPacket->xUDPHeader.usDestinationPort,
						&( xIsWaitingARPResolution ) ) == pdPASS )
	{
		/* xIsWaitingARPResolution is currently unused */
	}
	else
	{
		vReleaseNetworkBufferAndDescriptor( pxBuf );
		return;
	}


}


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

		xReturn = xNetworkQueueInsertPacketByFilter( &xItem, uxTimeout );
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

			if( xNetworkQueuePop( pxQueue, &xItem, 0 ) != pdFAIL )
			{

				#if ( tsnconfigCONTROLLER_HAS_DYNAMIC_PRIO != tsnconfigDISABLE )
					if( xNetworkQueueIsEmpty( pxQueue ) )
					{
						vTSNControllerComputePriority();
					}
				#endif

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
						xSendEventStructToIPTask( &xEvent, tsnconfigDEFAULT_QUEUE_TIMEOUT );
					}
					else
					{
						FreeRTOS_debug_printf( ("[%lu]Received: %32s\n", xTaskGetTickCount(), pxBuf->pucEthernetBuffer) );
						/*TODO: deliver to tasks */
						prvDeliverFrame( pxBuf );
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
	#if ( tsnconfigCONTROLLER_HAS_DYNAMIC_PRIO != tsnconfigDISABLE )
		if( uxTaskPriorityGet( xTSNControllerHandle ) < uxPriority )
		{
			vTaskPrioritySet( xTSNControllerHandle, uxPriority );
			return pdTRUE;
		}
	#endif

	return pdFALSE;
}

BaseType_t xIsCallingFromTSNController( void )
{
	return ( xTaskGetCurrentTaskHandle() == xTSNControllerHandle ) ? pdTRUE : pdFALSE;
}