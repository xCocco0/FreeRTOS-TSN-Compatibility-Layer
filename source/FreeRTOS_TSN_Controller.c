
#include "FreeRTOS.h"

#include "FreeRTOS_IP.h"
#include "FreeRTOS_IP_Private.h"

#include "FreeRTOSTSNConfig.h"
#include "FreeRTOSTSNConfigDefaults.h"

#include "FreeRTOS_TSN_Controller.h"
#include "FreeRTOS_TSN_NetworkScheduler.h"
#include "FreeRTOS_TSN_VLANTags.h"
#include "FreeRTOS_TSN_Sockets.h"
#include "NetworkWrapper.h"

#if ( tsnconfigCONTROLLER_HAS_DYNAMIC_PRIO != tsnconfigDISABLE )
	#define controllerTSN_TASK_BASE_PRIO ( tskIDLE_PRIORITY + 1 )
#else
	#define controllerTSN_TASK_BASE_PRIO ( tsnconfigTSN_CONTROLLER_PRIORITY )
#endif


static TaskHandle_t xTSNControllerHandle;

extern NetworkQueueList_t * pxNetworkQueueList;

void prvReceiveUDPPacketTSN( NetworkQueueItem_t * pxItem, TSNSocket_t xTSNSocket, Socket_t xBaseSocket )
{
	//FreeRTOS_TSN_Socket_t * const pxTSNSocket = ( FreeRTOS_TSN_Socket_t * ) xTSNSocket;
	FreeRTOS_Socket_t * const pxBaseSocket = ( FreeRTOS_Socket_t * ) xBaseSocket;

	( void ) xTSNSocket;

	if( pxItem->pxMsgh != NULL )
	{
		if( pxItem->pxMsgh->msg_flags & FREERTOS_MSG_ERRQUEUE )
		{
			vReleaseNetworkBufferAndDescriptor( pxItem->pxBuf );
			xSocketErrorQueueInsert( xTSNSocket, pxItem->pxMsgh );
			return;
		}
		/* storing the ancillary msg inside the ethernet buffer and the ethernet
		 * payload inside the ancillary msg iov.
		 * Note: we must remember to revert it back before freeing the network descriptor
		 */
		if( xAncillaryMsgFillPayload( pxItem->pxMsgh, pxItem->pxBuf->pucEthernetBuffer, pxItem->pxBuf->xDataLength ) != pdFAIL )
		{
			pxItem->pxBuf->pucEthernetBuffer = ( uint8_t * ) pxItem->pxMsgh;
		}
	}

	vTaskSuspendAll();
	vListInsertEnd( &( pxBaseSocket->u.xUDP.xWaitingPacketsList ), &( pxItem->pxBuf->xBufferListItem ) );
	( void ) xTaskResumeAll();


	/* Set the socket's receive event */
	if( pxBaseSocket->xEventGroup != NULL )
	{
		( void ) xEventGroupSetBits( pxBaseSocket->xEventGroup, ( EventBits_t ) eSOCKET_RECEIVE );
	}

	#if ( ipconfigSUPPORT_SELECT_FUNCTION == 1 )
	{
		if( ( pxBaseSocket->pxSocketSet != NULL ) && ( ( pxBaseSocket->xSelectBits & ( ( EventBits_t ) eSELECT_READ ) ) != 0U ) )
		{
			( void ) xEventGroupSetBits( pxBaseSocket->pxBaseSocketSet->xSelectGroup, ( EventBits_t ) eSELECT_READ );
		}
	}
	#endif

	#if ( ipconfigSOCKET_HAS_USER_SEMAPHORE == 1 )
	{
		if( pxBaseSocket->pxUserSemaphore != NULL )
		{
			( void ) xSemaphoreGive( pxBaseSocket->pxUserSemaphore );
		}
	}
	#endif
}

void prvDeliverFrame( NetworkQueueItem_t * pxItem )
{
	EthernetHeader_t * pxEthernetHeader;
	UDPPacket_t * pxUDPPacket;
	BaseType_t xIsWaitingARPResolution;
	NetworkBufferDescriptor_t * const pxBuf = pxItem->pxBuf;
	Socket_t xBaseSocket;
	TSNSocket_t xTSNSocket = FREERTOS_TSN_INVALID_SOCKET;

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

	#if ( tsnconfigSOCKET_INSERTS_VLAN_TAGS != tsnconfigDISABLE )
	ucTagsSize = sizeof( struct xVLAN_TAG ) * ucGetNumberOfTags( pxBuf );

	/* this will remove the vlan tags for reusing the
	 * FreeRTOS plus TCP functions */
	if( ucTagsSize > 0 )
	{
		memmove( &pxBuf->pucEthernetBuffer[ vlantagETH_TAG_OFFSET ],
				&pxBuf->pucEthernetBuffer[ vlantagETH_TAG_OFFSET + ucTagsSize ],
				pxBuf->xDataLength - vlantagETH_TAG_OFFSET );
	}
	#endif
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
			pxBuf->usBoundPort = pxUDPPacket->xUDPHeader.usDestinationPort;
        	pxBuf->xIPAddress.ulIP_IPv4 = pxUDPPacket->xIPHeader.ulSourceIPAddress;

			break;
			
		case ipIPv6_FRAME_TYPE:
			/* IPv6 support TODO */
		case ipARP_FRAME_TYPE:
		default:
			vReleaseNetworkBufferAndDescriptor( pxBuf );
			return;
	}

	vSocketFromPort( pxBuf->usBoundPort, &xBaseSocket, &xTSNSocket );
	
	if( xTSNSocket != FREERTOS_TSN_INVALID_SOCKET )
	{
		prvReceiveUDPPacketTSN( pxItem, xTSNSocket, xBaseSocket );
	}
	else
	{
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


}

static void prvTSNController( void * pvParameters )
{
	NetworkQueueItem_t xItem;
	IPStackEvent_t xEvent;
	NetworkBufferDescriptor_t * pxBuf;
	NetworkInterface_t * pxInterface;
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

				pxBuf = ( NetworkBufferDescriptor_t * ) xItem.pxBuf;

				/* for debugging */
				if( xItem.eEventType == eNetworkTxEvent )
				{
					pxInterface = pxBuf->pxEndPoint->pxNetworkInterface;
					FreeRTOS_debug_printf( ("[%lu]Sending: %32s\n", xTaskGetTickCount(), pxBuf->pucEthernetBuffer) );

					if( xItem.pxMsgh != NULL )
					{
						vAncillaryMsgFree( xItem.pxMsgh );
					}

					pxInterface->pfOutput( pxInterface, pxBuf, xItem.xReleaseAfterSend );
				}
				else
				{
					/* Receive of other events */
					if( pxQueue->ePolicy == eIPTaskEvents )
					{
						xEvent.eEventType = xItem.eEventType;
						xEvent.pvData = xItem.pxBuf;

						if( xItem.pxMsgh != NULL )
						{
							vAncillaryMsgFree( xItem.pxMsgh );
						}

						FreeRTOS_debug_printf( ("[%lu]Forwarding to IP task: %32s\n", xTaskGetTickCount(), pxBuf->pucEthernetBuffer) );
						xSendEventStructToIPTask( &xEvent, tsnconfigDEFAULT_QUEUE_TIMEOUT );
					}
					else
					{
						FreeRTOS_debug_printf( ("[%lu]Received: %32s\n", xTaskGetTickCount(), pxBuf->pucEthernetBuffer) );

						prvDeliverFrame( &xItem );
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