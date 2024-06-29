/**
 * @file FreeRTOS_TSN_Sockets.c
 * @brief FreeRTOS TSN Compatibility Layer - Socket Functions
 *
 * This file implements an alternative sockets API that works in parallel with
 * +TCP sockets. This sockets have an extended set of features that are missing
 * the original Addon but are essential when used within a time sensitive
 * network.
 */

#include "FreeRTOS.h"

#include "list.h"

#include "FreeRTOS_IP.h"
#include "FreeRTOS_IP_Private.h"
#include "FreeRTOS_ARP.h"

#include "FreeRTOS_TSN_Sockets.h"
#include "FreeRTOS_TSN_NetworkScheduler.h"
#include "FreeRTOS_TSN_VLANTags.h"
#include "FreeRTOS_TSN_DS.h"

/* private definitions from FreeRTOS_Sockets.c */
#define tsnsocketSET_SOCKET_PORT( pxSocket, usPort )    listSET_LIST_ITEM_VALUE( ( &( ( pxSocket )->xBoundSocketListItem ) ), ( usPort ) )
#define tsnsocketGET_SOCKET_PORT( pxSocket )            listGET_LIST_ITEM_VALUE( ( &( ( pxSocket )->xBoundSocketListItem ) ) )
#define tsnsocketSOCKET_IS_BOUND( pxSocket )            ( listLIST_ITEM_CONTAINER( &( pxSocket )->xBoundSocketListItem ) != NULL )

/* @brief List of bound TSN sockets
 *
 * TSN sockets are stored with key their port, in network byte order. Note that
 * TSN sockets are listed here, and their underlining Plus TCP sockets are also
 * listed in xBoundSocketListItem inside Plus TCP.
 */
static List_t xTSNBoundUDPSocketList;

void vInitialiseTSNSockets()
{
    if( !listLIST_IS_INITIALISED( &xTSNBoundUDPSocketList ) )
    {
        vListInitialise( &xTSNBoundUDPSocketList );
    }
}

BaseType_t xSocketErrorQueueInsert( TSNSocket_t xTSNSocket,
                                    struct msghdr * pxMsgh )
{
    FreeRTOS_TSN_Socket_t * pxTSNSocket = ( FreeRTOS_TSN_Socket_t * ) xTSNSocket;

    return xQueueSend( pxTSNSocket->xErrQueue, &pxMsgh, 0 );
}


/**
 * @brief Searches for a socket based on a given search key and retrieves the corresponding base socket and TSN socket.
 *
 * This function searches for a socket in the TSN bound UDP socket list based on the provided search key.
 * If a matching socket is found, the corresponding TSN socket and base socket are retrieved.
 *
 * @param xSearchKey The search key used to find the socket.
 * @param pxBaseSocket Pointer to the base socket variable where the retrieved base socket will be stored.
 * @param pxTSNSocket Pointer to the TSN socket variable where the retrieved TSN socket will be stored.
 */
void vSocketFromPort( TickType_t xSearchKey,
                      Socket_t * pxBaseSocket,
                      TSNSocket_t * pxTSNSocket )
{
    FreeRTOS_Socket_t ** ppxBaseSocket = ( FreeRTOS_Socket_t ** ) pxBaseSocket;
    FreeRTOS_TSN_Socket_t ** ppxTSNSocket = ( FreeRTOS_TSN_Socket_t ** ) pxTSNSocket;

    const ListItem_t * pxIterator;

    // Check if the TSN bound UDP socket list is initialized
    if( !listLIST_IS_INITIALISED( &( xTSNBoundUDPSocketList ) ) )
    {
        return;
    }

    const ListItem_t * pxEnd = ( ( const ListItem_t * ) &( xTSNBoundUDPSocketList.xListEnd ) );

    // Iterate through the TSN bound UDP socket list
    for( pxIterator = listGET_NEXT( pxEnd );
         pxIterator != pxEnd;
         pxIterator = listGET_NEXT( pxIterator ) )
    {
        // Check if the search key matches the value of the current list item
        if( listGET_LIST_ITEM_VALUE( pxIterator ) == xSearchKey )
        {
            // Retrieve the TSN socket and base socket
            *ppxTSNSocket = ( FreeRTOS_TSN_Socket_t * ) listGET_LIST_ITEM_OWNER( pxIterator );
            *ppxBaseSocket = ( *ppxTSNSocket )->xBaseSocket;
            return;
        }
    }

    // If no matching socket is found, perform a lookup in the UDP socket list
    *ppxBaseSocket = pxUDPSocketLookup( xSearchKey );
    *ppxTSNSocket = NULL;
}

/**
 * @brief Prepare a buffer for sending UDPv4 packets.
 *
 * This function prepares a buffer for sending UDPv4 packets. It sets the necessary headers,
 * including Ethernet, IP, and UDP headers, and performs ARP cache lookup to obtain the
 * destination MAC address.
 *
 * @param pxSocket The TSN socket.
 * @param pxBuf The network buffer descriptor.
 * @param xFlags The flags for the send operation.
 * @param pxDestinationAddress The destination address.
 * @param xDestinationAddressLength The length of the destination address.
 * @return pdPASS if the buffer is prepared successfully, pdFAIL otherwise.
 */
BaseType_t prvPrepareBufferUDPv4( FreeRTOS_TSN_Socket_t * pxSocket,
                                  NetworkBufferDescriptor_t * pxBuf,
                                  BaseType_t xFlags,
                                  const struct freertos_sockaddr * pxDestinationAddress,
                                  BaseType_t xDestinationAddressLength )
{
    EthernetHeader_t * pxEthernetHeader;
    IPHeader_t * pxIPHeader;
    UDPHeader_t * pxUDPHeader;
    eARPLookupResult_t eReturned;
    NetworkEndPoint_t * pxEndPoint;

    // Calculate the VLAN offset based on the number of VLAN tags
    #if ( tsnconfigSOCKET_INSERTS_VLAN_TAGS != tsnconfigDISABLE )
        const size_t uxVLANOffset = sizeof( struct xVLAN_TAG ) * pxSocket->ucVLANTagsCount;
    #else
        const size_t uxVLANOffset = 0;
    #endif

    // Calculate the payload size
    const size_t uxPayloadSize = pxBuf->xDataLength - sizeof( UDPPacket_t ) - uxVLANOffset;

    pxEthernetHeader = ( EthernetHeader_t * ) pxBuf->pucEthernetBuffer;

    // Reset the destination MAC address to avoid random garbage matches in the ARP tables
    memset( &pxEthernetHeader->xDestinationAddress, '\0', sizeof( MACAddress_t ) );

    // Perform ARP cache lookup to obtain the destination MAC address
    eReturned = eARPGetCacheEntry( &( pxBuf->xIPAddress.ulIP_IPv4 ), &( pxEthernetHeader->xDestinationAddress ), &( pxEndPoint ) );

    if( eReturned != eARPCacheHit )
    {
        FreeRTOS_debug_printf( ( "sendto: IP is not in ARP cache\n" ) );
        return pdFAIL;
    }

    pxBuf->pxEndPoint = pxEndPoint;

    // Copy the source MAC address from the endpoint to the Ethernet header
    ( void ) memcpy( pxEthernetHeader->xSourceAddress.ucBytes, pxEndPoint->xMACAddress.ucBytes, ( size_t ) ipMAC_ADDRESS_LENGTH_BYTES );

    // Set the VLAN tags in the Ethernet header based on the number of VLAN tags
    #if ( tsnconfigSOCKET_INSERTS_VLAN_TAGS != tsnconfigDISABLE )
        switch( pxSocket->ucVLANTagsCount )
        {
            case 0:
                // No VLAN tags, can reuse previous struct
                pxEthernetHeader->usFrameType = ipIPv4_FRAME_TYPE;
                break;

            case 1:
                // Single VLAN tag
                TaggedEthernetHeader_t * pxTEthHeader = ( TaggedEthernetHeader_t * ) pxEthernetHeader;
                pxTEthHeader->xVLANTag.usTPID = FreeRTOS_htons( vlantagTPID_DEFAULT );
                pxTEthHeader->xVLANTag.usTCI = FreeRTOS_htons( pxSocket->usVLANCTagTCI );
                pxTEthHeader->usFrameType = ipIPv4_FRAME_TYPE;
                break;

            case 2:
                // Double VLAN tag
                DoubleTaggedEthernetHeader_t * pxDTEthHeader = ( DoubleTaggedEthernetHeader_t * ) pxEthernetHeader;
                pxDTEthHeader->xVLANSTag.usTPID = FreeRTOS_htons( vlantagTPID_DOUBLE_TAG );
                pxDTEthHeader->xVLANSTag.usTCI = FreeRTOS_htons( pxSocket->usVLANSTagTCI );
                pxDTEthHeader->xVLANCTag.usTPID = FreeRTOS_htons( vlantagTPID_DEFAULT );
                pxDTEthHeader->xVLANCTag.usTCI = FreeRTOS_htons( pxSocket->usVLANCTagTCI );
                pxDTEthHeader->usFrameType = ipIPv4_FRAME_TYPE;
                break;

            default:
                return pdFAIL;
        }
    #else
        // No VLAN tags
        pxEthernetHeader->usFrameType = ipIPv4_FRAME_TYPE;
    #endif

    // Set the IP header
    pxIPHeader = ( IPHeader_t * ) &pxBuf->pucEthernetBuffer[ ipSIZE_OF_ETH_HEADER + uxVLANOffset ];
    pxIPHeader->ucVersionHeaderLength = ipIPV4_VERSION_HEADER_LENGTH_MIN;
    diffservSET_DSCLASS_IPv4( pxIPHeader, pxSocket->ucDSClass );
    pxIPHeader->usLength = uxPayloadSize + sizeof( IPHeader_t ) + sizeof( UDPHeader_t );
    pxIPHeader->usLength = FreeRTOS_htons( pxIPHeader->usLength );
    pxIPHeader->usIdentification = 0;
    pxIPHeader->usFragmentOffset = 0;
    pxIPHeader->ucTimeToLive = ipconfigUDP_TIME_TO_LIVE;
    pxIPHeader->ucProtocol = ipPROTOCOL_UDP;
    pxIPHeader->ulSourceIPAddress = pxBuf->pxEndPoint->ipv4_settings.ulIPAddress;
    pxIPHeader->ulDestinationIPAddress = pxBuf->xIPAddress.ulIP_IPv4;

    // Set the UDP header
    pxUDPHeader = ( UDPHeader_t * ) &pxBuf->pucEthernetBuffer[ ipSIZE_OF_ETH_HEADER + uxVLANOffset + ipSIZE_OF_IPv4_HEADER ];
    pxUDPHeader->usDestinationPort = pxBuf->usPort;
    pxUDPHeader->usSourcePort = pxBuf->usBoundPort;
    pxUDPHeader->usLength = ( uint16_t ) ( uxPayloadSize + sizeof( UDPHeader_t ) );
    pxUDPHeader->usLength = FreeRTOS_htons( pxUDPHeader->usLength );

    // Calculate the IP and UDP checksums
    #if ( ipconfigDRIVER_INCLUDED_TX_IP_CHECKSUM == 0 )
    {
        pxIPHeader->usHeaderChecksum = 0U;
        pxIPHeader->usHeaderChecksum = usGenerateChecksum( 0U, ( uint8_t * ) &( pxIPHeader ), ipSIZE_OF_IPv4_HEADER );
        pxIPHeader->usHeaderChecksum = ( uint16_t ) ~FreeRTOS_htons( pxIPHeader->usHeaderChecksum );

        if( ( ucSocketOptions & ( uint8_t ) FREERTOS_SO_UDPCKSUM_OUT ) != 0U )
        {
            // Shift the Ethernet buffer forward so that ethertype is at fixed offset from start
            ( void ) usGenerateProtocolChecksum( ( uint8_t * ) &pxBuf->pucEthernetBuffer[ uxVLANOffset ], pxNetworkBuffer->xDataLength - uxVLANOffset, pdTRUE );
        }
        else
        {
            pxUDPPacket->xUDPHeader.usChecksum = 0U;
        }
    }
    #else
    {
        pxIPHeader->usHeaderChecksum = 0U;
        pxUDPHeader->usChecksum = 0U;
    }
    #endif

    // Add padding if the data length is less than the minimum packet bytes
    #if ( ipconfigETHERNET_MINIMUM_PACKET_BYTES > 0 )
    {
        if( pxBuf->xDataLength < ( size_t ) ipconfigETHERNET_MINIMUM_PACKET_BYTES )
        {
            BaseType_t xIndex;

            for( xIndex = ( BaseType_t ) pxBuf->xDataLength; xIndex < ( BaseType_t ) ipconfigETHERNET_MINIMUM_PACKET_BYTES; xIndex++ )
            {
                pxBuf->pucEthernetBuffer[ xIndex ] = 0U;
            }

            pxBuf->xDataLength = ( size_t ) ipconfigETHERNET_MINIMUM_PACKET_BYTES;
        }
    }
    #endif

    return pdPASS;
}

/**
 * @brief Prepares a UDPv6 buffer for transmission.
 *
 * This function prepares a buffer for UDPv6 transmission by initializing the necessary data structures
 * and copying the destination address into the packet header.
 *
 * @param pxSocket The socket to which the buffer belongs.
 * @param pxBuf The network buffer descriptor to be prepared.
 * @param xFlags Flags to control the behavior of the function.
 * @param pxDestinationAddress Pointer to the destination address structure.
 * @param xDestinationAddressLength Length of the destination address.
 *
 * @return pdFAIL if the buffer preparation fails, pdPASS otherwise.
 */
BaseType_t prvPrepareBufferUDPv6( FreeRTOS_TSN_Socket_t * pxSocket,
                                  NetworkBufferDescriptor_t * pxBuf,
                                  BaseType_t xFlags,
                                  const struct freertos_sockaddr * pxDestinationAddress,
                                  BaseType_t xDestinationAddressLength )
{
	/* UDP on IPv6 is not yet supported. This function is never called because
	 * trying to create a socket with IPPROTO as IPv6 will always fail
	 */
	( void ) pxSocket;
	( void ) pxBuf;
	( void ) xFlags;
	( void ) pxDestinationAddress;
	( void ) xDestinationAddressLength;

    //UDPPacket_IPv6_t * pxUDPPacket;

    /*( void ) memcpy( pxUDPPacket_IPv6->xIPHeader.xDestinationAddress.ucBytes, pxDestinationAddress->sin_address.xIP_IPv6.ucBytes, ipSIZE_OF_IPv6_ADDRESS ); */


    return pdFAIL;
}

/**
 * @brief Moves the buffer pointer to the start of the payload and updates the payload size.
 *
 * This function is used to move the buffer pointer to the start of the payload and update the payload size
 * based on the frame type and protocol. It supports IPv4 and IPv6 frames with UDP, TCP, ICMP, ICMPv6, and IGMP protocols.
 *
 * @param[in,out] ppvBuf Pointer to the buffer pointer.
 * @param[in,out] puxSize Pointer to the payload size.
 */
void prvMoveToStartOfPayload( void ** ppvBuf,
                              size_t * puxSize )
{
    uint8_t * const pucEthernetBuffer = ( uint8_t * ) *ppvBuf;
    size_t uxPrefix = ipSIZE_OF_ETH_HEADER;
    uint8_t ucProtocol;

    const uint16_t usFrameType = ( ( EthernetHeader_t * ) *ppvBuf )->usFrameType;

    switch( usFrameType )
    {
        case ipIPv4_FRAME_TYPE:
            ucProtocol = ( ( IPHeader_t * ) &pucEthernetBuffer[ uxPrefix ] )->ucProtocol;
            uxPrefix += ipSIZE_OF_IPv4_HEADER;
            break;

        case ipIPv6_FRAME_TYPE:
            ucProtocol = ( ( IPHeader_IPv6_t * ) &pucEthernetBuffer[ uxPrefix ] )->ucNextHeader;
            uxPrefix += ipSIZE_OF_IPv6_HEADER;
            break;

        default:
            ucProtocol = 255;
            break;
    }

    switch( ucProtocol )
    {
        case ipPROTOCOL_UDP:
            *puxSize = FreeRTOS_ntohs( ( ( UDPHeader_t * ) &pucEthernetBuffer[ uxPrefix ] )->usLength );
            uxPrefix += ipSIZE_OF_UDP_HEADER;
            break;

        case ipPROTOCOL_TCP:
        case ipPROTOCOL_ICMP:
        case ipPROTOCOL_ICMP_IPv6:
        case ipPROTOCOL_IGMP:
        default:
            /* shouldn't get here, but just in case return raw packet */
            return;
    }

    *ppvBuf = &pucEthernetBuffer[ uxPrefix ];
}

/**
 * @brief Creates a TSN socket.
 *
 * This function creates a TSN socket with the specified domain, type, and protocol.
 * Only UDP sockets are supported at the moment.
 *
 * @param xDomain   The domain of the socket.
 * @param xType     The type of the socket.
 * @param xProtocol The protocol of the socket.
 *
 * @return The created TSN socket, or FREERTOS_TSN_INVALID_SOCKET if an error occurred.
 */
TSNSocket_t FreeRTOS_TSN_socket(BaseType_t xDomain,
                                BaseType_t xType,
                                BaseType_t xProtocol)
{
    // Check if the socket type is supported
    if (xType != FREERTOS_SOCK_DGRAM)
    {
        /* Only UDP is supported up to now */
        return FREERTOS_TSN_INVALID_SOCKET;
    }

    // Allocate memory for the TSN socket
    TSNSocket_t pxSocket = (TSNSocket_t)pvPortMalloc(sizeof(FreeRTOS_TSN_Socket_t));

    if (pxSocket == NULL)
    {
        return FREERTOS_TSN_INVALID_SOCKET;
    }

    // Create the underlying base socket
    pxSocket->xBaseSocket = FreeRTOS_socket(xDomain, xType, xProtocol);

    if (pxSocket->xBaseSocket == FREERTOS_INVALID_SOCKET)
    {
        vPortFree(pxSocket);
        return FREERTOS_TSN_INVALID_SOCKET;
    }

    // Create the error queue
    pxSocket->xErrQueue = xQueueCreate(tsnconfigERRQUEUE_LENGTH, sizeof(struct msghdr *));

    if (pxSocket->xErrQueue == NULL)
    {
        vPortFree(pxSocket);
        return FREERTOS_TSN_INVALID_SOCKET;
    }

    // Initialize the bound socket list item
    vListInitialiseItem(&(pxSocket->xBoundSocketListItem));
    listSET_LIST_ITEM_OWNER(&(pxSocket->xBoundSocketListItem), (void *)pxSocket);

    return pxSocket;
}

/**
 * @brief Set socket options for a TSN socket.
 *
 * This function sets various options for a TSN socket.
 *
 * @param xSocket The TSN socket to set options for.
 * @param lLevel The level at which the option is defined.
 * @param lOptionName The name of the option to set.
 * @param pvOptionValue A pointer to the value of the option.
 * @param uxOptionLength The length of the option value.
 *
 * @return pdPASS if the option is set successfully, or a negative value if an error occurs.
 */
BaseType_t FreeRTOS_TSN_setsockopt( TSNSocket_t xSocket,
                                    int32_t lLevel,
                                    int32_t lOptionName,
                                    const void * pvOptionValue,
                                    size_t uxOptionLength )
{
    BaseType_t xReturn = -pdFREERTOS_ERRNO_EINVAL;
    FreeRTOS_TSN_Socket_t * pxSocket = ( FreeRTOS_TSN_Socket_t * ) xSocket;
    BaseType_t ulOptionValue = *( BaseType_t * ) pvOptionValue;

    if( xSocketValid( pxSocket->xBaseSocket ) == pdTRUE )
    {
        switch( lOptionName )
        {
            #if ( tsnconfigSOCKET_INSERTS_VLAN_TAGS != tsnconfigDISABLE )
                case FREERTOS_SO_VLAN_CTAG_PCP:

                    // Set the PCP value from the TCI
                    if( ulOptionValue < 8 )
                    {
                        vlantagSET_PCP_FROM_TCI( pxSocket->usVLANCTagTCI, ulOptionValue );

                        // If no VLAN tags are set, set the count to 1
                        if( pxSocket->ucVLANTagsCount == 0 )
                        {
                            pxSocket->ucVLANTagsCount = 1;
                        }

                        xReturn = 0;
                    }

                    break;

                case FREERTOS_SO_VLAN_STAG_PCP:

                    // Set the PCP value from the TCI
                    if( ulOptionValue < 8 )
                    {
                        vlantagSET_PCP_FROM_TCI( pxSocket->usVLANSTagTCI, ulOptionValue );

                        // Set the VLAN tags count to 2
                        pxSocket->ucVLANTagsCount = 2;
                        xReturn = 0;
                    }

                    break;

                case FREERTOS_SO_VLAN_TAG_RST:
                    // Reset the VLAN tags count
                    pxSocket->ucVLANTagsCount = 0;
                    xReturn = 0;
                    break;
            #endif /* if ( tsnconfigSOCKET_INSERTS_VLAN_TAGS != tsnconfigDISABLE ) */

			/* In feature may want to replace this to provide an interface
			 * similar to the Linux sockets' IP_TOS field.
			 */
            case FREERTOS_SO_DS_CLASS:

                // Set the DS class value
                if( ulOptionValue < 64 )
                {
                    pxSocket->ucDSClass = ulOptionValue;
                    xReturn = 0;
                }

                break;

            case FREERTOS_SO_TIMESTAMPING:

                // Set the timestamping flags
                if( ulOptionValue & ~SOF_TIMESTAMPING_MASK )
                {
                    xReturn = pdFREERTOS_ERRNO_EINVAL;
                }
                else
                {
                    // Reset the error queue and set the timestamping flags
                    ( void ) xQueueReset( pxSocket->xErrQueue );
                    pxSocket->ulTSFlags = ulOptionValue;
                    xReturn = 0;
                }

                break;

            default:
                // Call the base socket's setsockopt function
                xReturn = FreeRTOS_setsockopt( pxSocket->xBaseSocket, lLevel, lOptionName, pvOptionValue, uxOptionLength );
                break;
        }
    }
    else
    {
        xReturn = -pdFREERTOS_ERRNO_EINVAL;
    }

    return xReturn;
}

/**
 * @brief Binds a TSN socket to a specific address.
 *
 * This function binds a TSN socket to a specific address specified by `pxAddress`.
 * The `xAddressLength` parameter specifies the length of the address structure.
 *
 * @param xSocket The TSN socket to bind.
 * @param pxAddress Pointer to the address structure.
 * @param xAddressLength The length of the address structure.
 *
 * @return If the socket is successfully bound, the function returns 0. Otherwise, it returns a negative value.
 */
BaseType_t FreeRTOS_TSN_bind( TSNSocket_t xSocket,
                              struct freertos_sockaddr const * pxAddress,
                              socklen_t xAddressLength )
{
    BaseType_t xRet;
    FreeRTOS_TSN_Socket_t * pxSocket = ( FreeRTOS_TSN_Socket_t * ) xSocket;
    FreeRTOS_Socket_t * pxBaseSocket = ( FreeRTOS_Socket_t * ) pxSocket->xBaseSocket;

    // Bind the base socket
    xRet = FreeRTOS_bind( pxSocket->xBaseSocket, pxAddress, xAddressLength );

    if( xRet == 0 )
    {
        // Set the socket port
        tsnsocketSET_SOCKET_PORT( pxSocket, FreeRTOS_htons( pxBaseSocket->usLocalPort ) );

        // Insert the socket into the bound UDP socket list
        vListInsertEnd( &xTSNBoundUDPSocketList, &( pxSocket->xBoundSocketListItem ) );
    }

    return xRet;
}

/**
 * @brief Closes a TSN socket.
 *
 * This function closes the specified TSN socket.
 *
 * @param xSocket The TSN socket to be closed.
 *
 * @return If the socket is successfully closed, the function returns 0. Otherwise, it returns an error code.
 */
BaseType_t FreeRTOS_TSN_closesocket( TSNSocket_t xSocket )
{
    FreeRTOS_TSN_Socket_t * pxSocket = ( FreeRTOS_TSN_Socket_t * ) xSocket;

    /*FreeRTOS_Socket_t * pxBaseSocket = ( FreeRTOS_Socket_t * ) pxSocket->xBaseSocket; */

    // Check if the socket is invalid
    if( xSocket == FREERTOS_TSN_INVALID_SOCKET )
    {
        return 0;
    }

    // Remove the socket from the bound socket list
    ( void ) uxListRemove( &( pxSocket->xBoundSocketListItem ) );

    // Close the base socket
    return FreeRTOS_closesocket( pxSocket->xBaseSocket );
}

/**
 * @brief Sends data to a TSN socket.
 *
 * This function sends data to a TSN socket specified by the `xSocket` parameter.
 *
 * @param xSocket The TSN socket to send data to.
 * @param pvBuffer Pointer to the data buffer containing the data to send.
 * @param uxTotalDataLength The total length of the data to send.
 * @param xFlags Flags to control the behavior of the send operation.
 * @param pxDestinationAddress Pointer to the destination address structure.
 * @param xDestinationAddressLength The length of the destination address structure.
 *
 * @return The number of bytes sent on success, or a negative error code on failure.
 */
int32_t FreeRTOS_TSN_sendto( TSNSocket_t xSocket,
                             const void * pvBuffer,
                             size_t uxTotalDataLength,
                             BaseType_t xFlags,
                             const struct freertos_sockaddr * pxDestinationAddress,
                             socklen_t xDestinationAddressLength )
{
    FreeRTOS_TSN_Socket_t * pxSocket = ( FreeRTOS_TSN_Socket_t * ) xSocket;
    FreeRTOS_Socket_t * pxBaseSocket = ( FreeRTOS_Socket_t * ) pxSocket->xBaseSocket;
    TimeOut_t xTimeOut;
    TickType_t xRemainingTime = pxBaseSocket->xSendBlockTime;
    NetworkQueueItem_t xEvent;
    size_t uxMaxPayloadLength, uxPayloadOffset;
    NetworkBufferDescriptor_t * pxBuf;

    vTaskSetTimeOutState( &xTimeOut );

    if( pxDestinationAddress == NULL )
    {
        FreeRTOS_debug_printf( ( "sendto: invalid destination address\n" ) );
        return -pdFREERTOS_ERRNO_EINVAL;
    }

    if( !tsnsocketSOCKET_IS_BOUND( pxBaseSocket ) )
    {
        FreeRTOS_debug_printf( ( "sendto: socket is not bound\n" ) );
        return -pdFREERTOS_ERRNO_EBADF;
    }

    switch( pxDestinationAddress->sin_family )
    {
        #if ( ipconfigUSE_IPv6 != 0 )
            case FREERTOS_AF_INET6:

				/* Note: This part is to be revised and completed. For now it
				 * will always fail after prvPrepareBufferUDPv6().
				 */

                uxPayloadOffset = ipSIZE_OF_ETH_HEADER + ipSIZE_OF_IPv6_HEADER + ipSIZE_OF_UDP_HEADER;
                #if ( tsnconfigSOCKET_INSERTS_VLAN_TAGS != tsnconfigDISABLE )
                    uxPayloadOffset += pxSocket->ucVLANTagsCount * sizeof( struct xVLAN_TAG );
                #endif
                uxMaxPayloadLength = ipconfigNETWORK_MTU - ( ipSIZE_OF_IPv6_HEADER + ipSIZE_OF_UDP_HEADER );
                pxBuf = pxGetNetworkBufferWithDescriptor( uxTotalDataLength + uxPayloadOffset, pxBaseSocket->xSendBlockTime );

                if( pxBuf == NULL )
                {
                    FreeRTOS_debug_printf( ( "sendto: couldn't acquire network buffer\n" ) );
                    return -pdFREERTOS_ERRNO_EAGAIN;
                }

                pxBuf->xDataLength = uxTotalDataLength + uxPayloadOffset;
                pxBuf->pxEndPoint = pxBaseSocket->pxEndPoint;
                pxBuf->usPort = pxDestinationAddress->sin_port;
                pxBuf->usBoundPort = ( uint16_t ) tsnsocketGET_SOCKET_PORT( pxBaseSocket );
                ( void ) memcpy( pxBuf->xIPAddress.xIP_IPv6.ucBytes, pxDestinationAddress->sin_address.xIP_IPv6.ucBytes, ipSIZE_OF_IPv6_ADDRESS );

                if( prvPrepareBufferUDPv6( pxSocket, pxBuf, xFlags, pxDestinationAddress, xDestinationAddressLength ) != pdPASS )
                {
                    vReleaseNetworkBufferAndDescriptor( pxBuf );
                    return -pdFREERTOS_ERRNO_EAGAIN;
                }
                break;
        #endif /* ( ipconfigUSE_IPv6 != 0 ) */

        #if ( ipconfigUSE_IPv4 != 0 )
            case FREERTOS_AF_INET4:
                uxPayloadOffset = ipSIZE_OF_ETH_HEADER + ipSIZE_OF_IPv4_HEADER + ipSIZE_OF_UDP_HEADER;
                #if ( tsnconfigSOCKET_INSERTS_VLAN_TAGS != tsnconfigDISABLE )
                    uxPayloadOffset += pxSocket->ucVLANTagsCount * sizeof( struct xVLAN_TAG );
                #endif
                uxMaxPayloadLength = ipconfigNETWORK_MTU - ( ipSIZE_OF_IPv4_HEADER + ipSIZE_OF_UDP_HEADER );
                pxBuf = pxGetNetworkBufferWithDescriptor( uxTotalDataLength + uxPayloadOffset, pxBaseSocket->xSendBlockTime );

                if( pxBuf == NULL )
                {
                    FreeRTOS_debug_printf( ( "sendto: couldn't acquire network buffer\n" ) );
                    return -pdFREERTOS_ERRNO_EAGAIN;
                }

                pxBuf->xDataLength = uxTotalDataLength + uxPayloadOffset;
                pxBuf->pxEndPoint = pxBaseSocket->pxEndPoint;
                pxBuf->usPort = pxDestinationAddress->sin_port;
                pxBuf->usBoundPort = ( uint16_t ) tsnsocketGET_SOCKET_PORT( pxBaseSocket );
                pxBuf->xIPAddress.ulIP_IPv4 = pxDestinationAddress->sin_address.ulIP_IPv4;

                if( prvPrepareBufferUDPv4( pxSocket, pxBuf, xFlags, pxDestinationAddress, xDestinationAddressLength ) != pdPASS )
                {
                    vReleaseNetworkBufferAndDescriptor( pxBuf );
                    return -pdFREERTOS_ERRNO_EAGAIN;
                }
                break;
        #endif /* ( ipconfigUSE_IPv4 != 0 ) */

        default:
            FreeRTOS_debug_printf( ( "sendto: invalid sin familyl\n" ) );
            return -pdFREERTOS_ERRNO_EINVAL;
    }

    if( uxTotalDataLength > uxMaxPayloadLength )
    {
        FreeRTOS_debug_printf( ( "sendto: payload is too large\n" ) );
        vReleaseNetworkBufferAndDescriptor( pxBuf );
        return -pdFREERTOS_ERRNO_EINVAL;
    }

    memcpy( &pxBuf->pucEthernetBuffer[ uxPayloadOffset ], pvBuffer, uxTotalDataLength );

    xEvent.eEventType = eNetworkTxEvent;
    xEvent.pxBuf = ( void * ) pxBuf;
    xEvent.pxMsgh = NULL;
    xEvent.xReleaseAfterSend = pdTRUE;

    if( xTaskCheckForTimeOut( &xTimeOut, &xRemainingTime ) != pdTRUE )
    {
        if( xNetworkQueueInsertPacketByFilter( &xEvent, xRemainingTime ) == pdPASS )
        {
            return uxTotalDataLength;
        }
        else
        {
            FreeRTOS_debug_printf( ( "sendto: cannot insert into network queues\n" ) );
            vReleaseNetworkBufferAndDescriptor( pxBuf );
            return -pdFREERTOS_ERRNO_EAGAIN;
        }
    }
    else
    {
        FreeRTOS_debug_printf( ( "sendto: timeout occurred\n" ) );
        vReleaseNetworkBufferAndDescriptor( pxBuf );
        return -pdFREERTOS_ERRNO_ETIMEDOUT;
    }
}

/**
 * @brief Receives a message from a TSN socket.
 *
 * This function receives a message from the specified TSN socket. It retrieves the message
 * from the waiting packets list of the underlying base socket. If the `FREERTOS_MSG_ERRQUEUE`
 * flag is set, it retrieves the message from the error queue of the TSN socket.
 *
 * @param xSocket The TSN socket from which to receive the message.
 * @param pxMsghUser Pointer to the `msghdr` structure that will hold the received message.
 * @param xFlags Flags that control the behavior of the receive operation.
 *
 * @return The length of the payload of the received message, or a negative error code if an error occurs.
 */
int32_t FreeRTOS_TSN_recvmsg( TSNSocket_t xSocket,
                              struct msghdr * pxMsghUser,
                              BaseType_t xFlags )
{
    FreeRTOS_TSN_Socket_t * pxSocket = ( FreeRTOS_TSN_Socket_t * ) xSocket;
    FreeRTOS_Socket_t * pxBaseSocket = ( FreeRTOS_Socket_t * ) pxSocket->xBaseSocket;

    // Check if the socket is valid
    if( ( pxSocket == NULL ) || ( pxSocket == FREERTOS_TSN_INVALID_SOCKET ) )
    {
        return -pdFREERTOS_ERRNO_EINVAL;
    }

    // Check if the base socket is bound
    if( !tsnsocketSOCKET_IS_BOUND( pxBaseSocket ) )
    {
        return -pdFREERTOS_ERRNO_EINVAL;
    }

    // Check if the msghdr pointer is valid
    if( pxMsghUser == NULL )
    {
        return -pdFREERTOS_ERRNO_EINVAL;
    }

    BaseType_t xTimed = pdFALSE;
    TickType_t xRemainingTime = pxBaseSocket->xReceiveBlockTime;
    BaseType_t lPacketCount;
    TimeOut_t xTimeOut;
    NetworkBufferDescriptor_t * pxNetworkBuffer = NULL;
    struct msghdr * pxMsgh;
    size_t uxPayloadLen = 0;
    size_t uxLen;

    // Get the number of packets waiting to be processed by the socket
    lPacketCount = ( BaseType_t ) listCURRENT_LIST_LENGTH( &( pxBaseSocket->u.xUDP.xWaitingPacketsList ) );

    // Check if the FREERTOS_MSG_ERRQUEUE flag is set
    if( xFlags & FREERTOS_MSG_ERRQUEUE )
    {
        // Try to receive a message from the error queue
        if( xQueueReceive( pxSocket->xErrQueue, &pxMsgh, 0 ) == pdFALSE )
        {
            return -pdFREERTOS_ERRNO_EWOULDBLOCK;
        }
    }
    else
    {
        // If there are no packets waiting, wait for a packet to arrive
        while( lPacketCount == 0 )
        {
            // Check if the socket is non-blocking
            if( xTimed == pdFALSE )
            {
                // Check if the remaining time has expired
                if( xRemainingTime == ( TickType_t ) 0 )
                {
                    break;
                }

                // Check if the FREERTOS_MSG_DONTWAIT flag is set
                if( ( ( ( UBaseType_t ) xFlags ) & ( ( UBaseType_t ) FREERTOS_MSG_DONTWAIT ) ) != 0U )
                {
                    break;
                }

                // Set the timed flag and fetch the current time
                xTimed = pdTRUE;
                vTaskSetTimeOutState( &xTimeOut );
            }

            // Wait for a packet to arrive or an interrupt to occur
            ( void ) xEventGroupWaitBits( pxBaseSocket->xEventGroup, ( ( EventBits_t ) eSOCKET_RECEIVE ) | ( ( EventBits_t ) eSOCKET_INTR ),
                                          pdTRUE /*xClearOnExit*/, pdFALSE /*xWaitAllBits*/, xRemainingTime );

            // Update the packet count
            lPacketCount = ( BaseType_t ) listCURRENT_LIST_LENGTH( &( pxBaseSocket->u.xUDP.xWaitingPacketsList ) );

            // If a packet has arrived, break the loop
            if( lPacketCount != 0 )
            {
                break;
            }

            // Check if the timeout has been reached
            if( xTaskCheckForTimeOut( &xTimeOut, &xRemainingTime ) != pdFALSE )
            {
                break;
            }
        } /* while( lPacketCount == 0 ) */

        // If there are packets waiting, retrieve the first packet
        if( lPacketCount > 0 )
        {
            vTaskSuspendAll();
            {
                // Get the network buffer associated with the first packet
                pxNetworkBuffer = ( ( NetworkBufferDescriptor_t * ) listGET_OWNER_OF_HEAD_ENTRY( &( pxBaseSocket->u.xUDP.xWaitingPacketsList ) ) );

                // Remove the network buffer from the list if not peeking
                if( ( ( UBaseType_t ) xFlags & ( UBaseType_t ) FREERTOS_MSG_PEEK ) == 0U )
                {
                    ( void ) uxListRemove( &( pxNetworkBuffer->xBufferListItem ) );
                }
            }
            ( void ) xTaskResumeAll();
        }

        // If no packet is available, return an error
        if( pxNetworkBuffer == NULL )
        {
            return -pdFREERTOS_ERRNO_EWOULDBLOCK;
        }

        // Get the msghdr structure from the network buffer
        pxMsgh = ( struct msghdr * ) pxNetworkBuffer->pucEthernetBuffer;
        pxNetworkBuffer->pucEthernetBuffer = ( uint8_t * ) pxMsgh->msg_iov[ 0 ].iov_base;
    }

    // Copy the message name if available
    if( ( pxMsgh->msg_name != NULL ) && ( pxMsghUser->msg_name != NULL ) )
    {
        uxLen = configMIN( pxMsghUser->msg_namelen, pxMsgh->msg_namelen );
        memcpy( pxMsghUser->msg_name, pxMsgh->msg_name, uxLen );
        pxMsghUser->msg_namelen = uxLen;
    }
    else
    {
        pxMsghUser->msg_namelen = 0;
    }

    // Copy the message payload if available
    if( ( pxMsgh->msg_iov != NULL ) && ( pxMsghUser->msg_iov != NULL ) )
    {
        prvMoveToStartOfPayload( &( pxMsgh->msg_iov[ 0 ].iov_base ), &( pxMsgh->msg_iov[ 0 ].iov_len ) );

        pxMsghUser->msg_iovlen = configMIN( pxMsghUser->msg_iovlen, pxMsgh->msg_iovlen );

        // Copy each element of the message payload array
        for( BaseType_t uxIter = 0; uxIter < pxMsghUser->msg_iovlen; ++uxIter )
        {
            uxLen = configMIN( pxMsghUser->msg_iov[ uxIter ].iov_len, pxMsgh->msg_iov[ uxIter ].iov_len );
            memcpy( pxMsghUser->msg_iov[ uxIter ].iov_base, pxMsgh->msg_iov[ uxIter ].iov_base, uxLen );
            pxMsghUser->msg_iov[ uxIter ].iov_len = uxLen;
            uxPayloadLen += uxLen;
        }
    }
    else
    {
        pxMsghUser->msg_iovlen = 0;
    }

    // Copy the message control if available
    if( ( pxMsghUser->msg_control != NULL ) && ( pxMsgh->msg_control != NULL ) )
    {
        uxLen = configMIN( pxMsghUser->msg_controllen, pxMsgh->msg_controllen );
        memcpy( pxMsghUser->msg_control, pxMsgh->msg_control, uxLen );
        pxMsghUser->msg_controllen = uxLen;
    }
    else
    {
        pxMsghUser->msg_controllen = 0;
    }

    // Copy the message flags
    pxMsghUser->msg_flags = pxMsgh->msg_flags;

    // Free any ancillary messages
    vAncillaryMsgFreeAll( pxMsgh );

    // Release the network buffer
    if( pxNetworkBuffer != NULL )
    {
        vReleaseNetworkBufferAndDescriptor( pxNetworkBuffer );
    }

    // Return the length of the payload
    return uxPayloadLen;
}

/**
 * @brief Receive data from a TSN socket.
 *
 * This function receives data from a TSN socket and stores it in the provided buffer.
 *
 * @param xSocket The TSN socket to receive data from.
 * @param pvBuffer Pointer to the buffer where the received data will be stored.
 * @param uxBufferLength The length of the buffer in bytes.
 * @param xFlags Flags to control the behavior of the receive operation.
 * @param pxSourceAddress Pointer to a structure that will hold the source address information.
 * @param pxSourceAddressLength Pointer to the length of the source address structure.
 *
 * @return The number of bytes received on success, or a negative error code on failure.
 */
int32_t FreeRTOS_TSN_recvfrom( TSNSocket_t xSocket,
                               void * pvBuffer,
                               size_t uxBufferLength,
                               BaseType_t xFlags,
                               struct freertos_sockaddr * pxSourceAddress,
                               socklen_t * pxSourceAddressLength )
{
    struct msghdr xMsghdr;
    struct iovec xIovec;

    // Set the source address and its length in the msghdr structure
    xMsghdr.msg_name = pxSourceAddress;
    xMsghdr.msg_namelen = sizeof( struct freertos_sockaddr );

    /* errors are not copied from the original msg */
    xMsghdr.msg_control = NULL;
    xMsghdr.msg_controllen = 0;

    // Set the buffer and its length in the iovec structure
    xIovec.iov_base = pvBuffer;
    xIovec.iov_len = uxBufferLength;

    // Set the iovec structure in the msghdr structure
    xMsghdr.msg_iov = &xIovec;
    xMsghdr.msg_iovlen = 1;

    /* use instead recvmsg() to receive on errqueue */
    xFlags &= ~FREERTOS_MSG_ERRQUEUE;

    // Call the FreeRTOS_TSN_recvmsg() function to receive the data
    return FreeRTOS_TSN_recvmsg( xSocket, &xMsghdr, xFlags );
}
