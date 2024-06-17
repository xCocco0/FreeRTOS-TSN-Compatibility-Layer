/* Wrap around NetworkInterface.c but rename drivers functions and
 * hijack signals to IPTasks to our TSN Controller task
 */
#include "NetworkWrapper.h"
#include "FreeRTOS_TSN_Controller.h"
#include "FreeRTOS_TSN_NetworkScheduler.h"
#include "FreeRTOS_TSN_Sockets.h"
#include "FreeRTOS_TSN_VLANTags.h"
#include "FreeRTOS_TSN_Timestamp.h"

#define xNetworkInterfaceInitialise    xMAC_NetworkInterfaceInitialise
#define xNetworkInterfaceOutput        xMAC_NetworkInterfaceOutput
#define xGetPhyLinkStatus              xMAC_GetPhyLinkStatus
#define pxFillInterfaceDescriptor      pxMAC_FillInterfaceDescriptor

extern BaseType_t xSendEventStructToTSNController( const IPStackEvent_t * pxEvent,
                                                   TickType_t uxTimeout );
#define xSendEventStructToIPTask    xSendEventStructToTSNController

#include "NetworkInterface.c"

#undef pxFillInterfaceDescriptor
#undef xNetworkInterfaceInitialise
#undef xNetworkInterfaceOutput
#undef xGetPhyLinkStatus

#undef xSendEventStructToIPTask

/* Private function definitions
 */
NetworkBufferDescriptor_t * prvInsertVLANTag( NetworkBufferDescriptor_t * const pxBuf,
                                              uint16_t usTCI,
                                              uint16_t usTPID );

NetworkQueueItem_t * prvHandleReceive( NetworkBufferDescriptor_t * pxBuf );


#define wrapperFIRST_TPID     ( 0x88a8 )
#define wrapperSECOND_TPID    ( 0x8100 )

NetworkBufferDescriptor_t * prvInsertVLANTag( NetworkBufferDescriptor_t * const pxBuf,
                                              uint16_t usTCI,
                                              uint16_t usTPID )
{
    NetworkBufferDescriptor_t * pxNewBuf;
    const size_t uxTagSize = sizeof( usTCI ) + sizeof( usTPID );
    const size_t uxNewSize = pxBuf->xDataLength + uxTagSize;

    pxNewBuf = pxResizeNetworkBufferWithDescriptor( pxBuf, uxNewSize );

    if( pxNewBuf == NULL )
    {
        vReleaseNetworkBufferAndDescriptor( pxBuf );
        return NULL;
    }

    pxNewBuf->xDataLength = uxNewSize;
    const size_t uxPrefix = offsetof( EthernetHeader_t, usFrameType );
    uint8_t * const pucBuffer = pxNewBuf->pucEthernetBuffer;
    usTCI = FreeRTOS_htons( usTCI );
    usTPID = FreeRTOS_htons( usTPID );

    memmove( &pucBuffer[ uxPrefix + uxTagSize ], &pucBuffer[ uxPrefix ], pxBuf->xDataLength - uxPrefix );
    memcpy( &pucBuffer[ uxPrefix ], &usTPID, sizeof( usTPID ) );
    memcpy( &pucBuffer[ uxPrefix + sizeof( usTPID ) ], &usTCI, sizeof( usTCI ) );

    return pxNewBuf;
}

BaseType_t prvStripVLANTag( NetworkBufferDescriptor_t * pxBuf,
                            uint16_t * pusVLANTCI,
                            uint16_t * pusVLANServiceTCI )
{
    BaseType_t uxNumTags = ucGetNumberOfTags( pxBuf );
    const size_t uxStart = offsetof( EthernetHeader_t, usFrameType );
    struct xVLAN_TAG * const pxTag = ( struct xVLAN_TAG * ) &pxBuf->pucEthernetBuffer[ uxStart ];

    switch( uxNumTags )
    {
        case 0:
            return 0;

        case 1:
            *pusVLANTCI = pxTag->usTCI;
            break;

        case 2:
        default:
            *pusVLANServiceTCI = pxTag[ 0 ].usTCI;
            *pusVLANTCI = pxTag[ 1 ].usTCI;
            break;
    }

    const size_t uxTagSize = uxNumTags * sizeof( struct xVLAN_TAG );

    memmove( &pxBuf->pucEthernetBuffer[ uxStart ],
             &pxBuf->pucEthernetBuffer[ uxStart + uxTagSize ],
             pxBuf->xDataLength - uxTagSize - uxStart );

    pxBuf->xDataLength -= uxTagSize;

    return uxNumTags;
}

BaseType_t prvAncillaryMsgControlFillForRx( struct msghdr * pxMsgh,
                                            NetworkBufferDescriptor_t * pxBuf,
                                            Socket_t xSocket,
                                            TSNSocket_t xTSNSocket )
{
    FreeRTOS_TSN_Socket_t * const pxTSNSocket = ( FreeRTOS_TSN_Socket_t * ) xTSNSocket;
    /*FreeRTOS_Socket_t * const pxBaseSocket = ( FreeRTOS_Socket_t * ) pxTSNSocket->xBaseSocket; */

    struct cmsghdr xControlMsg[ 1 ];
    void * pucControlData[ 1 ];
    size_t uxPayloadSize[ 1 ];
    struct freertos_scm_timestamping xTimestamp;
    BaseType_t xOptions = 0;

    if( pxTSNSocket != NULL )
    {
        if( pxTSNSocket->ulTSFlags & SOF_TIMESTAMPING_RX_SOFTWARE && ( xTimebaseGetState() == eTimebaseEnabled ) )
        {
            uxPayloadSize[ xOptions ] = sizeof( xTimestamp );
            xControlMsg[ xOptions ].cmsg_len = CMSG_LEN( uxPayloadSize[ xOptions ] );
            xControlMsg[ xOptions ].cmsg_level = FREERTOS_SOL_SOCKET;
            xControlMsg[ xOptions ].cmsg_type = FREERTOS_SCM_TIMESTAMPING;
            vTimestampAcquireSoftware( &xTimestamp.ts[ 0 ] );
            xTimestamp.ts[ 1 ].tv_sec = 0;
            xTimestamp.ts[ 1 ].tv_nsec = 0;
            vRetrieveHardwareTimestamp( pxBuf->pxInterface, pxBuf, &xTimestamp.ts[ 2 ].tv_sec, &xTimestamp.ts[ 2 ].tv_nsec );
            pucControlData[ xOptions ] = &xTimestamp;
            ++xOptions;
        }

        /* Other eventual options... */
        /* ++xOptions; ... */
    }

    /* Other eventual options... */

    if( xOptions > 0 )
    {
        if( xAncillaryMsgControlFill( pxMsgh, xControlMsg, pucControlData, uxPayloadSize, xOptions ) != pdPASS )
        {
            /* no memory for filling control msgs*/
            return 0;
        }
    }
    else
    {
        pxMsgh->msg_control = NULL;
        pxMsgh->msg_controllen = 0;
    }

    return xOptions;
}

BaseType_t prvAncillaryMsgControlFillForTx( struct msghdr * pxMsgh,
                                            NetworkBufferDescriptor_t * pxBuf,
                                            Socket_t xSocket,
                                            TSNSocket_t xTSNSocket )
{
    FreeRTOS_TSN_Socket_t * const pxTSNSocket = ( FreeRTOS_TSN_Socket_t * ) xTSNSocket;
    FreeRTOS_Socket_t * const pxBaseSocket = ( FreeRTOS_Socket_t * ) pxTSNSocket->xBaseSocket;

    struct cmsghdr xControlMsg[ 2 ];
    void * pucControlData[ 2 ];
    size_t uxPayloadSize[ 2 ];
    struct freertos_scm_timestamping xTimestamp;
    struct sock_extended_err xSockErr;
    BaseType_t xOptions = 0;

    if( pxTSNSocket != NULL )
    {
        if( pxTSNSocket->ulTSFlags & SOF_TIMESTAMPING_TX_SOFTWARE )
        {
            uxPayloadSize[ xOptions ] = sizeof( xSockErr );
            xControlMsg[ xOptions ].cmsg_len = CMSG_LEN( uxPayloadSize[ xOptions ] );
            xControlMsg[ xOptions ].cmsg_level = pxBaseSocket->bits.bIsIPv6 ? FREERTOS_SOL_IPV6 : FREERTOS_SOL_IP;
            xControlMsg[ xOptions ].cmsg_type = pxBaseSocket->bits.bIsIPv6 ? FREERTOS_IPV6_RECVERR : FREERTOS_IP_RECVERR;
            xSockErr.ee_errno = pdFREERTOS_ERRNO_ENOMSG;
            xSockErr.ee_origin = SO_EE_ORIGIN_TXSTATUS;
            xSockErr.ee_info = SCM_TSTAMP_SND;
            xSockErr.ee_data = 0; /* SOF_TIMESTAMPING_OPT_ID unsupported yet */
            pucControlData[ xOptions ] = &xSockErr;
            ++xOptions;

            uxPayloadSize[ xOptions ] = sizeof( xTimestamp );
            xControlMsg[ xOptions ].cmsg_len = CMSG_LEN( uxPayloadSize[ xOptions ] );
            xControlMsg[ xOptions ].cmsg_level = FREERTOS_SOL_SOCKET;
            xControlMsg[ xOptions ].cmsg_type = FREERTOS_SCM_TIMESTAMPING;
            vTimestampAcquireSoftware( &xTimestamp.ts[ 0 ] );
            xTimestamp.ts[ 1 ].tv_sec = 0;
            xTimestamp.ts[ 1 ].tv_nsec = 0;
            vRetrieveHardwareTimestamp( pxBuf->pxInterface, pxBuf, &xTimestamp.ts[ 2 ].tv_sec, &xTimestamp.ts[ 2 ].tv_nsec );
            pucControlData[ xOptions ] = &xTimestamp;
            ++xOptions;
        }

        /* Other eventual options... */
        /* ++xOptions; ... */
    }

    /* Other eventual options... */

    if( xOptions > 0 )
    {
        if( xAncillaryMsgControlFill( pxMsgh, xControlMsg, pucControlData, uxPayloadSize, xOptions ) != pdPASS )
        {
            /* no memory for filling control msgs*/
            return 0;
        }

        pxMsgh->msg_flags |= FREERTOS_MSG_ERRQUEUE;
    }
    else
    {
        pxMsgh->msg_control = NULL;
        pxMsgh->msg_controllen = 0;
    }

    return xOptions;
}

/* Network interface wrapper function definitions
 */

BaseType_t xTSN_NetworkInterfaceInitialise( NetworkInterface_t * pxInterface )
{
    return xMAC_NetworkInterfaceInitialise( pxInterface );
}

BaseType_t xTSN_NetworkInterfaceOutput( NetworkInterface_t * pxInterface,
                                        NetworkBufferDescriptor_t * const pxBuffer,
                                        BaseType_t bReleaseAfterSend )
{
    NetworkQueueItem_t xItem;
    NetworkInterfaceConfig_t * pxInterfaceConfig;
    Socket_t xSocket = NULL;
    TSNSocket_t xTSNSocket = NULL;
    NetworkBufferDescriptor_t * pxNewBuffer;
    struct msghdr * pxMsgh;

    /* Only the TSN controller is allowed to send packets to the MAC.
     * If the caller is the TSN controller proceed, otherwise put the message
     * in queue and wait for the controller to serve it.
     */
    if( xIsCallingFromTSNController() )
    {
        #if ( tsnconfigWRAPPER_INSERTS_VLAN_TAGS != tsnconfigDISABLE )
            pxInterfaceConfig = ( NetworkInterfaceConfig_t * ) pxInterface->pvArgument;

            if( pxInterfaceConfig->xNumTags >= 1 )
            {
                pxNewBuffer = prvInsertVLANTag( pxBuffer, pxInterfaceConfig->usVLANTag, wrapperSECOND_TPID );

                if( pxNewBuffer == NULL )
                {
                    return pdFALSE;
                }

                if( pxInterfaceConfig->xNumTags >= 2 )
                {
                    pxNewBuffer = prvInsertVLANTag( pxNewBuffer, pxInterfaceConfig->usServiceVLANTag, wrapperFIRST_TPID );

                    if( pxNewBuffer == NULL )
                    {
                        return pdFALSE;
                    }
                }
            }
        #else  /* if ( tsnconfigWRAPPER_INSERTS_VLAN_TAGS != tsnconfigDISABLE ) */
              /* The driver will insert the VLAN tag */
        #endif /* if ( tsnconfigWRAPPER_INSERTS_VLAN_TAGS != tsnconfigDISABLE ) */

        if( xMAC_NetworkInterfaceOutput( pxInterface, pxBuffer, bReleaseAfterSend ) != pdFAIL )
        {
            vSocketFromPort( pxBuffer->usBoundPort, &xSocket, &xTSNSocket );

            if( xTSNSocket != NULL )
            {
                pxMsgh = pxAncillaryMsgMalloc();

                if( pxMsgh != NULL )
                {
                    if( prvAncillaryMsgControlFillForTx( pxMsgh, pxBuffer, xSocket, xTSNSocket ) > 0 )
                    {
                        ( void ) xAncillaryMsgFillName( pxMsgh, NULL, 0, 0 );

                        /* pxMsgh->msg_iov is empty, unlikely from linux */

                        ( void ) xSocketErrorQueueInsert( xTSNSocket, pxMsgh );
                    }
                    else
                    {
                        vAncillaryMsgFree( pxMsgh );
                    }
                }
            }

            return pdPASS;
        }
        else
        {
            /* in future we can eventually pass an error cmsg here */
            return pdFAIL;
        }
    }

    /*
     * else if( xIsCallingFromIPTask() ) ...*/
    else
    {
        xItem.eEventType = eNetworkTxEvent;
        xItem.pxBuf = ( void * ) pxBuffer;
        xItem.xReleaseAfterSend = bReleaseAfterSend;
        return xNetworkQueueInsertPacketByFilter( &xItem, tsnconfigDEFAULT_QUEUE_TIMEOUT );
    }
}

NetworkInterface_t * pxTSN_FillInterfaceDescriptor( BaseType_t xEMACIndex,
                                                    NetworkInterface_t * pxInterface,
                                                    NetworkInterfaceConfig_t * pxInterfaceConfig )
{
    static char pcName[ 17 ];

    snprintf( pcName, sizeof( pcName ), "eth%u", ( unsigned ) xEMACIndex );

    memset( pxInterface, '\0', sizeof( *pxInterface ) );
    pxInterface->pcName = pcName; /* Just for logging, debugging. */
    pxInterface->pvArgument = ( void * ) pxInterfaceConfig;
    pxInterface->pfInitialise = xTSN_NetworkInterfaceInitialise;
    pxInterface->pfOutput = xTSN_NetworkInterfaceOutput;
    pxInterface->pfGetPhyLinkStatus = xTSN_GetPhyLinkStatus;

    pxInterfaceConfig->xEMACIndex = xEMACIndex;
    pxInterfaceConfig->xNumTags = 0;
    pxInterfaceConfig->usVLANTag = 0;

    FreeRTOS_AddNetworkInterface( pxInterface );

    return pxInterface;
}

BaseType_t xTSN_GetPhyLinkStatus( NetworkInterface_t * pxInterface )
{
    return xMAC_GetPhyLinkStatus( pxInterface );
}


/* Overwrite the default network interface with the previous
 */

BaseType_t xNetworkInterfaceInitialise( NetworkInterface_t * pxInterface )
{
    return xMAC_NetworkInterfaceInitialise( pxInterface );
}

BaseType_t xNetworkInterfaceOutput( NetworkInterface_t * pxInterface,
                                    NetworkBufferDescriptor_t * const pxBuffer,
                                    BaseType_t bReleaseAfterSend )
{
    return xMAC_NetworkInterfaceOutput( pxInterface, pxBuffer, bReleaseAfterSend );
}

#if defined( ipconfigIPv4_BACKWARD_COMPATIBLE ) && ( ipconfigIPv4_BACKWARD_COMPATIBLE == 1 )
    NetworkInterface_t * pxFillInterfaceDescriptor( BaseType_t xEMACIndex,
                                                    NetworkInterface_t * pxInterface )
    {
        return pxMAC_FillInterfaceDescriptor( xEMACIndex, pxInterface );
    }
#endif

BaseType_t xGetPhyLinkStatus( NetworkInterface_t * pxInterface )
{
    return xMAC_GetPhyLinkStatus( pxInterface );
}

/* Other functions
 */

void vRetrieveHardwareTimestamp( NetworkInterface_t * pxInterface,
                                 NetworkBufferDescriptor_t * pxBuf,
                                 uint32_t * pusSec,
                                 uint32_t * pusNanosec )
{
    /* No standard API in current FreeRTOS network drivers.
     * Leave the field empty for now.
     */

    ( void ) pxInterface;
    ( void ) pxBuf;
    *pusSec = 0;
    *pusNanosec = 0;
}

/**
 * @brief Function to send an event structure to the TSN Controller
 *
 * This function is responsible for sending an event structure to the TSN Controller.
 * If the event is a network receive event, it is inserted into the network queue.
 * Otherwise, it is sent to the IP task.
 *
 * @param[in] pxEvent Pointer to the IP stack event structure
 * @param[in] uxTimeout Timeout value for sending the event
 * @return pdTRUE if the event is sent successfully, pdFALSE otherwise
 */
BaseType_t xSendEventStructToTSNController( const IPStackEvent_t * pxEvent,
                                            TickType_t uxTimeout )
{
    BaseType_t xReturn = pdFALSE;
    NetworkQueueItem_t * pxItem;

    if( pxEvent->eEventType == eNetworkRxEvent )
    {
        pxItem = prvHandleReceive( ( NetworkBufferDescriptor_t * ) pxEvent->pvData );

        if( pxItem == NULL )
        {
            return pdFAIL;
        }

        xReturn = xNetworkQueueInsertPacketByFilter( pxItem, uxTimeout );
    }
    else
    {
        xReturn = xSendEventStructToIPTask( pxEvent, uxTimeout );
    }

    return xReturn;
}


NetworkQueueItem_t * prvHandleReceive( NetworkBufferDescriptor_t * pxBuf )
{
    NetworkQueueItem_t * pxItem;
    struct msghdr * pxMsgh;
    Socket_t xSocket;
    TSNSocket_t xTSNSocket;
    uint16_t usVLANTCI, usVLANServiceTCI, usFrameType, usDestinationPort = 0;
    uint8_t ucProtocol;
    size_t uxPrefix = 0;
    uint8_t * pucEBuf = pxBuf->pucEthernetBuffer;
    IP_Address_t xDestinationAddr;
    BaseType_t uxNumVLANTags;

    uxNumVLANTags = prvStripVLANTag( pxBuf, &usVLANTCI, &usVLANServiceTCI );
    usFrameType = ( ( EthernetHeader_t * ) pucEBuf )->usFrameType;
    uxPrefix = ipSIZE_OF_ETH_HEADER;

    switch( usFrameType )
    {
        case ipIPv4_FRAME_TYPE:
            ucProtocol = ( ( IPHeader_t * ) &pucEBuf[ uxPrefix ] )->ucProtocol;
            xDestinationAddr.ulIP_IPv4 = ( ( IPHeader_t * ) &pucEBuf[ uxPrefix ] )->ulDestinationIPAddress;
            uxPrefix += ipSIZE_OF_IPv4_HEADER;
            break;

        case ipIPv6_FRAME_TYPE:
            ucProtocol = ( ( IPHeader_IPv6_t * ) &pucEBuf[ uxPrefix ] )->ucNextHeader;
            xDestinationAddr.xIP_IPv6 = ( ( IPHeader_IPv6_t * ) &pucEBuf[ uxPrefix ] )->xDestinationAddress;
            uxPrefix += ipSIZE_OF_IPv6_HEADER;
            break;

        default:
            ucProtocol = 255;
            break;
    }

    switch( ucProtocol )
    {
        case ipPROTOCOL_UDP:
            usDestinationPort = ( ( UDPHeader_t * ) &pucEBuf[ uxPrefix ] )->usDestinationPort;
            uxPrefix += ipSIZE_OF_UDP_HEADER;
            break;

        case ipPROTOCOL_TCP:
            usDestinationPort = ( ( TCPHeader_t * ) &pucEBuf[ uxPrefix ] )->usDestinationPort;
            uxPrefix += ipSIZE_OF_TCP_HEADER;
            break;

        case ipPROTOCOL_ICMP:
        case ipPROTOCOL_ICMP_IPv6:
        case ipPROTOCOL_IGMP:
        default:
            break;
    }

    xSocket = NULL;

    vSocketFromPort( usDestinationPort, &xSocket, &xTSNSocket );

    if( xSocket != NULL )
    {
        pxMsgh = pxAncillaryMsgMalloc();

        if( pxMsgh != NULL )
        {
            ( void ) prvAncillaryMsgControlFillForRx( pxMsgh, pxBuf, xSocket, xTSNSocket );

            switch( usFrameType )
            {
                case ipIPv4_FRAME_TYPE:
                    ( void ) xAncillaryMsgFillName( pxMsgh, &xDestinationAddr, usDestinationPort, FREERTOS_AF_INET );
                    break;

                case ipIPv6_FRAME_TYPE:
                    ( void ) xAncillaryMsgFillName( pxMsgh, &xDestinationAddr, usDestinationPort, FREERTOS_AF_INET6 );
                    break;

                default:
                    break;
            }

            ( void ) xAncillaryMsgFillPayload( pxMsgh, pxBuf->pucEthernetBuffer, pxBuf->xDataLength );
        }
    }
    else
    {
        return NULL;
    }

    ( void ) uxNumVLANTags; /* TODO: do something with this */

    pxItem = pxNetworkQueueItemMalloc();

    if( pxItem == NULL )
    {
        return NULL;
    }

    pxItem->eEventType = eNetworkRxEvent;
    pxItem->pxBuf = pxBuf;
    pxItem->pxMsgh = pxMsgh;
    pxItem->xReleaseAfterSend = pdTRUE;

    return pxItem;
}
