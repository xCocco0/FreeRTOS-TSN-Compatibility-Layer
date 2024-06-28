/**
 * @file FreeRTOS_TSN_Controller.c
 * @brief FreeRTOS TSN Controller implementation
 *
 * This file contains the implementation of the FreeRTOS TSN Controller,
 * which is responsible for handling incoming network packets and forwarding
 * them to the appropriate tasks or the IP task.
 */

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
    #define controllerTSN_TASK_BASE_PRIO    ( tskIDLE_PRIORITY + 1 )
#else
    #define controllerTSN_TASK_BASE_PRIO    ( tsnconfigTSN_CONTROLLER_PRIORITY )
#endif


static TaskHandle_t xTSNControllerHandle = NULL;

extern NetworkQueueList_t * pxNetworkQueueList;

/**
 * @brief Receives a UDP packet for a TSN socket.
 *
 * This function is responsible for receiving a UDP packet for a TSN socket.
 * It handles error conditions and inserts the packet into the waiting packets list.
 * It also sets the receive event for the socket and updates the select group and user semaphore if applicable.
 *
 * @param pxItem Pointer to the NetworkQueueItem_t structure containing the received packet.
 * @param xTSNSocket The TSN socket to receive the packet for.
 * @param xBaseSocket The base socket associated with the TSN socket.
 */
void prvReceiveUDPPacketTSN( NetworkQueueItem_t * pxItem,
                             TSNSocket_t xTSNSocket,
                             Socket_t xBaseSocket )
{
    FreeRTOS_TSN_Socket_t * const pxTSNSocket = ( FreeRTOS_TSN_Socket_t * ) xTSNSocket;
    FreeRTOS_Socket_t * const pxBaseSocket = ( FreeRTOS_Socket_t * ) xBaseSocket;

    ( void ) pxTSNSocket;

    // Check if the message header is not NULL
    configASSERT( pxItem->pxMsgh != NULL );

    // Check if the message is an error queue message
    if( pxItem->pxMsgh->msg_flags & FREERTOS_MSG_ERRQUEUE )
    {
        // Release the network buffer and insert the error message into the socket's error queue
        if( pxItem->pxBuf != NULL )
        {
            vReleaseNetworkBufferAndDescriptor( pxItem->pxBuf );
        }

        xSocketErrorQueueInsert( xTSNSocket, pxItem->pxMsgh );
        return;
    }

    /* Passing the msghdr in the network buffer. Remember that the msghdr has
     * a reference to the ethernet buffer in the iovec. This must be reverted
     * back before releasing the network buffer!
     */
    // Check if the iov_base of the message header points to the ethernet buffer
    configASSERT( pxItem->pxMsgh->msg_iov[ 0 ].iov_base == pxItem->pxBuf->pucEthernetBuffer );
    // Update the ethernet buffer pointer to point to the message header
    pxItem->pxBuf->pucEthernetBuffer = ( uint8_t * ) pxItem->pxMsgh;

    // Suspend the scheduler to ensure atomicity
    vTaskSuspendAll();
    // Insert the network buffer into the waiting packets list of the base socket
    vListInsertEnd( &( pxBaseSocket->u.xUDP.xWaitingPacketsList ), &( pxItem->pxBuf->xBufferListItem ) );
    ( void ) xTaskResumeAll();

    /* Set the socket's receive event */
    if( pxBaseSocket->xEventGroup != NULL )
    {
        ( void ) xEventGroupSetBits( pxBaseSocket->xEventGroup, ( EventBits_t ) eSOCKET_RECEIVE );
    }

    #if ( ipconfigSUPPORT_SELECT_FUNCTION == 1 )
    {
        // Check if the socket is part of a socket set and if the read bit is set
        if( ( pxBaseSocket->pxSocketSet != NULL ) && ( ( pxBaseSocket->xSelectBits & ( ( EventBits_t ) eSELECT_READ ) ) != 0U ) )
        {
            // Set the read bit in the select group of the socket set
            ( void ) xEventGroupSetBits( pxBaseSocket->pxBaseSocketSet->xSelectGroup, ( EventBits_t ) eSELECT_READ );
        }
    }
    #endif

    #if ( ipconfigSOCKET_HAS_USER_SEMAPHORE == 1 )
    {
        // Check if the socket has a user semaphore
        if( pxBaseSocket->pxUserSemaphore != NULL )
        {
            // Give the user semaphore
            ( void ) xSemaphoreGive( pxBaseSocket->pxUserSemaphore );
        }
    }
    #endif
}

/**
 * @brief Function to deliver a network frame to the appropriate socket
 *
 * This function is responsible for delivering a network frame to the appropriate
 * socket based on the frame type.
 *
 * @param[in] pxBuf Pointer to the network buffer descriptor
 */
void prvDeliverFrame( NetworkQueueItem_t * pxItem,
                      BaseType_t xUsingIPTask )
{
    EthernetHeader_t * pxEthernetHeader;
    UDPPacket_t * pxUDPPacket;
    BaseType_t xIsWaitingARPResolution;
    NetworkBufferDescriptor_t * const pxBuf = pxItem->pxBuf;
    Socket_t xBaseSocket = NULL;
    TSNSocket_t xTSNSocket = NULL;
    IPStackEvent_t xEvent;

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
        /* this checks should be moved elsewhere in future, because this path
         * is in common with packets received using the IPTask and therefore
         * we are performing this checks twice
         */
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
            /*pxBuf->usPort = pxUDPPacket->xUDPHeader.usSourcePort;//already set in the wrapper */
            pxBuf->xIPAddress.ulIP_IPv4 = pxUDPPacket->xIPHeader.ulSourceIPAddress;

            break;

        case ipIPv6_FRAME_TYPE:
        /* IPv6 support TODO */
        case ipARP_FRAME_TYPE:
        default:
            break;
    }

    vSocketFromPort( pxBuf->usBoundPort, &xBaseSocket, &xTSNSocket );

    if( xTSNSocket != NULL )
    {
        /* Sending to a TSN socket
         */
        prvReceiveUDPPacketTSN( pxItem, xTSNSocket, xBaseSocket );
    }
    else if( xBaseSocket != NULL )
    {
        /* The receiving socket is a normal socket
         */
        if( pxItem->pxMsgh != NULL )
        {
            vAncillaryMsgFreeAll( pxItem->pxMsgh );
        }

        if( xUsingIPTask == pdTRUE )
        {
            xEvent.pvData = ( void * ) pxBuf;
            xEvent.eEventType = eNetworkRxEvent;

            if( xSendEventStructToIPTask( &xEvent, tsnconfigCONTROLLER_MAX_EVENT_WAIT ) == pdFAIL )
            {
                vReleaseNetworkBufferAndDescriptor( pxBuf );
                return;
            }
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
    else
    {
        /* No destination socket (i.e. ARP, ICMP...)
         */

        /* currently, no ARP or ICMP support in TSN extension, so pass it to
         * IP task and let +TCP do the work
         */
        xEvent.pvData = ( void * ) pxBuf;
        xEvent.eEventType = eNetworkRxEvent;

        if( xSendEventStructToIPTask( &xEvent, tsnconfigCONTROLLER_MAX_EVENT_WAIT ) == pdFAIL )
        {
            vReleaseNetworkBufferAndDescriptor( pxBuf );
            return;
        }
    }
}

/**
 * @brief TSN Controller task function
 *
 * This function is the entry point for the TSN Controller task.
 * It waits for notifications and processes network packets or events
 * based on the notification received.
 *
 * @param[in] pvParameters Pointer to the task parameters (not used)
 */
static void prvTSNController( void * pvParameters )
{
    NetworkQueueItem_t xItem; // Variable to hold the network queue item
    NetworkBufferDescriptor_t * pxBuf; // Pointer to the network buffer descriptor
    NetworkInterface_t * pxInterface; // Pointer to the network interface
    NetworkQueue_t * pxQueue; // Pointer to the network queue
    TickType_t uxTimeToSleep; // Time to sleep before processing notifications

    while( pdTRUE )
    {
        uxTimeToSleep = configMIN( uxNetworkQueueGetTicksUntilWakeup(), pdMS_TO_TICKS( tsnconfigCONTROLLER_MAX_EVENT_WAIT ) );
        /*configPRINTF( ( "[%lu] Sleeping for %lu ms\r\n", xTaskGetTickCount(), uxTimeToSleep ) ); */

        ulTaskNotifyTake( pdTRUE, uxTimeToSleep ); // Wait for a notification

        while( pdTRUE )
        {
            pxQueue = xNetworkQueueSchedule(); // Get the next network queue to process

            if( pxQueue == NULL )
            {
                break;
            }

            if( xNetworkQueuePop( pxQueue, &xItem, 0 ) != pdFAIL ) // Pop an item from the network queue
            {
                pxBuf = ( NetworkBufferDescriptor_t * ) xItem.pxBuf;

                if( xItem.eEventType == eNetworkTxEvent ) // If the event is a transmit event
                {
                    pxInterface = pxBuf->pxEndPoint->pxNetworkInterface;
                    /*FreeRTOS_debug_printf( ( "[%lu]Sending: %32s\n", xTaskGetTickCount(), pxBuf->pucEthernetBuffer ) ); */

                    if( xItem.pxMsgh != NULL )
                    {
                        /* note: this won't free the iov_base buffers, which is
                         * ok since we need the pxBuf ethernet buffer to be
                         * passed to the socket
                         * */
                        vAncillaryMsgFreeAll( xItem.pxMsgh ); // Free the ancillary message
                    }

                    pxInterface->pfOutput( pxInterface, pxBuf, xItem.xReleaseAfterSend ); // Output the network buffer
                }
                else
                {
                    /* Receive or other events */

                    /*FreeRTOS_debug_printf( ( "[%lu]Received: %32s\n", xTaskGetTickCount(), pxBuf->pucEthernetBuffer ) ); */

                    prvDeliverFrame( &xItem, pxQueue->ePolicy == eIPTaskEvents ? pdTRUE : pdFALSE ); // Deliver the network frame
                }

                #if ( tsnconfigCONTROLLER_HAS_DYNAMIC_PRIO != tsnconfigDISABLE )
                    if( xNetworkQueueIsEmpty( pxQueue ) )
                    {
                        vTSNControllerComputePriority(); // Compute the priority if the network queue is empty
                    }
                #endif
            }
        }
    }
}

/**
 * @brief Function to initialize the TSN Controller task
 *
 * This function creates the TSN Controller task and sets its priority.
 */
void vTSNController_Initialise( void )
{
	if ( xTSNControllerHandle == NULL )
	{
		xTaskCreate( prvTSNController,
					 "TSN-controller",
					 ipconfigIP_TASK_STACK_SIZE_WORDS,
					 NULL,
					 controllerTSN_TASK_BASE_PRIO,
					 &( xTSNControllerHandle ) );
	}
}

/**
 * @brief Function to notify the TSN Controller task
 *
 * This function notifies the TSN Controller task to wake up and process
 * pending network packets or events.
 *
 * @return pdTRUE if the notification is sent successfully, pdFALSE otherwise
 */
BaseType_t xNotifyController()
{
    return xTaskNotifyGive( xTSNControllerHandle );
}

/**
 * @brief Function to compute the priority of the TSN Controller task
 *
 * The priority of the TSN controller is the maximum IPV among all the queues
 * which has pending messages.
 */
void vTSNControllerComputePriority( void )
{
    NetworkQueueList_t * pxIter = pxNetworkQueueList;
    UBaseType_t uxPriority = controllerTSN_TASK_BASE_PRIO;

    while( pxIter != NULL )
    {
        if( !xNetworkQueueIsEmpty( pxIter->pxQueue ) )
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

/**
 * @brief Function to update the priority of the TSN Controller task
 *
 * This function updates the priority of the TSN Controller task if the
 * new priority is higher than the current priority.
 *
 * @param[in] uxPriority New priority for the TSN Controller task
 * @return pdTRUE if the priority is updated, pdFALSE otherwise
 */
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

/**
 * @brief Function to check if the caller task is the TSN Controller task
 *
 * This function checks if the caller task is the TSN Controller task.
 *
 * @return pdTRUE if the current task is the TSN Controller task, pdFALSE otherwise
 */
BaseType_t xIsCallingFromTSNController( void )
{
    return ( xTaskGetCurrentTaskHandle() == xTSNControllerHandle ) ? pdTRUE : pdFALSE;
}
