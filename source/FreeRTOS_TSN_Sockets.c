
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

/* List of bound TSN sockets, stored with key their port, in network byte order
*/
static List_t xTSNBoundUDPSocketList;

void vInitialiseTSNSockets()
{
	if( !listLIST_IS_INITIALISED( &xTSNBoundUDPSocketList ) ) {

		vListInitialise( &xTSNBoundUDPSocketList );
	}
}

BaseType_t xSocketErrorQueueInsert( TSNSocket_t xTSNSocket, struct msghdr * pxMsgh )
{
	FreeRTOS_TSN_Socket_t * pxTSNSocket = ( FreeRTOS_TSN_Socket_t * ) xTSNSocket;
	return xQueueSend( pxTSNSocket->xErrQueue , &pxMsgh, 0);
}


void vSocketFromPort( TickType_t xSearchKey, Socket_t * pxBaseSocket, TSNSocket_t * pxTSNSocket )
{
	FreeRTOS_Socket_t ** ppxBaseSocket = ( FreeRTOS_Socket_t ** ) pxBaseSocket;
	FreeRTOS_TSN_Socket_t ** ppxTSNSocket = ( FreeRTOS_TSN_Socket_t ** ) pxTSNSocket;

	const ListItem_t * pxIterator;

    if( !listLIST_IS_INITIALISED( &( xTSNBoundUDPSocketList ) ) )
	{
		return;
	}

	const ListItem_t * pxEnd = ( ( const ListItem_t * ) &( xTSNBoundUDPSocketList.xListEnd ) );

	for( pxIterator = listGET_NEXT( pxEnd );
			pxIterator != pxEnd;
			pxIterator = listGET_NEXT( pxIterator ) )
	{
		if( listGET_LIST_ITEM_VALUE( pxIterator ) == xSearchKey )
		{
			*ppxTSNSocket = ( FreeRTOS_TSN_Socket_t * ) listGET_LIST_ITEM_OWNER( pxIterator );
			*ppxBaseSocket = ( *ppxTSNSocket )->xBaseSocket;
			return;
		}
	}
	
	*ppxBaseSocket = pxUDPSocketLookup( xSearchKey );
	*ppxTSNSocket = NULL;

    return;
}

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
	#if ( tsnconfigSOCKET_INSERTS_VLAN_TAGS != tsnconfigDISABLE )
	const size_t uxVLANOffset = sizeof( struct xVLAN_TAG ) * pxSocket->ucVLANTagsCount;
	#else
	const size_t uxVLANOffset = 0;
	#endif
	const size_t uxPayloadSize = pxBuf->xDataLength - sizeof( UDPPacket_t ) - uxVLANOffset;

	pxEthernetHeader = ( EthernetHeader_t * ) pxBuf->pucEthernetBuffer;

	/* reset because it can happen that random garbage can find a match
	 * in the ARP tables */
	memset( &pxEthernetHeader->xDestinationAddress, '\0', sizeof( MACAddress_t ) );

	eReturned = eARPGetCacheEntry( &( pxBuf->xIPAddress.ulIP_IPv4 ), &( pxEthernetHeader->xDestinationAddress ), &( pxEndPoint ) );
	
	if( eReturned != eARPCacheHit )
	{
		FreeRTOS_debug_printf( ("sendto: IP is not in ARP cache\n") );
		return pdFAIL;
	}
	
	pxBuf->pxEndPoint = pxEndPoint;

	( void ) memcpy( pxEthernetHeader->xSourceAddress.ucBytes, pxEndPoint->xMACAddress.ucBytes, ( size_t ) ipMAC_ADDRESS_LENGTH_BYTES );

	#if ( tsnconfigSOCKET_INSERTS_VLAN_TAGS != tsnconfigDISABLE )
	switch( pxSocket->ucVLANTagsCount )
	{
		case 0: ;
			/* can reuse previous struct */
			pxEthernetHeader->usFrameType = ipIPv4_FRAME_TYPE;
			break;

		case 1: ;
			TaggedEthernetHeader_t * pxTEthHeader = ( TaggedEthernetHeader_t * ) pxEthernetHeader;
			pxTEthHeader->xVLANTag.usTPID = FreeRTOS_htons( vlantagTPID_DEFAULT );
			pxTEthHeader->xVLANTag.usTCI = FreeRTOS_htons( pxSocket->usVLANCTagTCI );
			pxTEthHeader->usFrameType = ipIPv4_FRAME_TYPE;
			break;

		case 2: ;
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
	pxEthernetHeader->usFrameType = ipIPv4_FRAME_TYPE;
	#endif

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
	
	pxUDPHeader = ( UDPHeader_t * ) &pxBuf->pucEthernetBuffer[ ipSIZE_OF_ETH_HEADER + uxVLANOffset + ipSIZE_OF_IPv4_HEADER ];

	pxUDPHeader->usDestinationPort = pxBuf->usPort;
	pxUDPHeader->usSourcePort = pxBuf->usBoundPort;
	pxUDPHeader->usLength = ( uint16_t ) ( uxPayloadSize + sizeof( UDPHeader_t ) );
	pxUDPHeader->usLength = FreeRTOS_htons( pxUDPHeader->usLength );

	#if ( ipconfigDRIVER_INCLUDED_TX_IP_CHECKSUM == 0 )
	{
		pxIPHeader->usHeaderChecksum = 0U;
		pxIPHeader->usHeaderChecksum = usGenerateChecksum( 0U, ( uint8_t * ) &( pxIPHeader ), ipSIZE_OF_IPv4_HEADER );
		pxIPHeader->usHeaderChecksum = ( uint16_t ) ~FreeRTOS_htons( pxIPHeader->usHeaderChecksum );

		if( ( ucSocketOptions & ( uint8_t ) FREERTOS_SO_UDPCKSUM_OUT ) != 0U )
		{
			/* shift the ethernet buffer forward so that ethertype is at fixed offset
			 * from start. It is ok since ethertype is the only eth field used */
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
	#endif /* if ( ipconfigDRIVER_INCLUDED_TX_IP_CHECKSUM == 0 ) */

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
	#endif /* if( ipconfigETHERNET_MINIMUM_PACKET_BYTES > 0 ) */

	return pdPASS;
}

BaseType_t prvPrepareBufferUDPv6( FreeRTOS_TSN_Socket_t * pxSocket, 
								NetworkBufferDescriptor_t * pxBuf,
								BaseType_t xFlags,
								const struct freertos_sockaddr * pxDestinationAddress,
								BaseType_t xDestinationAddressLength )
{
	UDPPacket_IPv6_t * pxUDPPacket;
    //( void ) memcpy( pxUDPPacket_IPv6->xIPHeader.xDestinationAddress.ucBytes, pxDestinationAddress->sin_address.xIP_IPv6.ucBytes, ipSIZE_OF_IPv6_ADDRESS );

	return pdFAIL;
}

TSNSocket_t FreeRTOS_TSN_socket( BaseType_t xDomain,
						  BaseType_t xType,
						  BaseType_t xProtocol )
{
	if( xType != FREERTOS_SOCK_DGRAM )
	{
		/* Only UDP is supported up to now */
		return FREERTOS_TSN_INVALID_SOCKET;
	}

	TSNSocket_t pxSocket = ( TSNSocket_t ) pvPortMalloc( sizeof( FreeRTOS_TSN_Socket_t ) );

	if( pxSocket == NULL )
	{
		return FREERTOS_TSN_INVALID_SOCKET;
	}

	pxSocket->xBaseSocket = FreeRTOS_socket( xDomain, xType, xProtocol );
	
	if( pxSocket->xBaseSocket == FREERTOS_INVALID_SOCKET )
	{
		vPortFree( pxSocket );
		return FREERTOS_TSN_INVALID_SOCKET;
	}
	
	pxSocket->xErrQueue = xQueueCreate( tsnconfigERRQUEUE_LENGTH, sizeof( struct msghdr * ) );

	if( pxSocket->xErrQueue == NULL )
	{
		vPortFree( pxSocket );
		return FREERTOS_TSN_INVALID_SOCKET;
	}

	vListInitialiseItem( &( pxSocket->xBoundSocketListItem ) );
	listSET_LIST_ITEM_OWNER( &( pxSocket->xBoundSocketListItem ), ( void * ) pxSocket );
	
	return pxSocket;
}

BaseType_t FreeRTOS_TSN_setsockopt( TSNSocket_t xSocket,
                                int32_t lLevel,
                                int32_t lOptionName,
                                const void * pvOptionValue,
                                size_t uxOptionLength )
{
    BaseType_t xReturn = -pdFREERTOS_ERRNO_EINVAL;
	FreeRTOS_TSN_Socket_t * pxSocket = ( FreeRTOS_TSN_Socket_t * ) xSocket;
	BaseType_t ulOptionValue = * ( BaseType_t * ) pvOptionValue;

    if( xSocketValid( pxSocket->xBaseSocket ) == pdTRUE )
	{
		switch( lOptionName )
		{
			#if ( tsnconfigSOCKET_INSERTS_VLAN_TAGS != tsnconfigDISABLE )
				case FREERTOS_SO_VLAN_CTAG_PCP:
					if( ulOptionValue < 8 )
					{
						vlantagSET_PCP_FROM_TCI( pxSocket->usVLANCTagTCI, ulOptionValue );
						if( pxSocket->ucVLANTagsCount == 0 )
						{
							pxSocket->ucVLANTagsCount = 1;
						}
						xReturn = 0;
					}
					break;

				case FREERTOS_SO_VLAN_STAG_PCP:
					if( ulOptionValue < 8 )
					{
						vlantagSET_PCP_FROM_TCI( pxSocket->usVLANSTagTCI, ulOptionValue );
						pxSocket->ucVLANTagsCount = 2;
						xReturn = 0;
					}
					break;
				
				case FREERTOS_SO_VLAN_TAG_RST:
					pxSocket->ucVLANTagsCount = 0;
					xReturn = 0;
					break;
			#endif

			case FREERTOS_SO_DS_CLASS:
				if( ulOptionValue < 64 )
				{
					pxSocket->ucDSClass = ulOptionValue;
					xReturn = 0;
				}
				break;

			case FREERTOS_SO_TIMESTAMPING:
					
				if ( ulOptionValue & ~SOF_TIMESTAMPING_MASK )
				{
					xReturn = pdFREERTOS_ERRNO_EINVAL;
				}
				else
				{
					pxSocket->ulTSFlags = ulOptionValue;
					xReturn = 0;
				}
				break;
			
			default:
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

BaseType_t FreeRTOS_TSN_bind( TSNSocket_t xSocket,
                          struct freertos_sockaddr const * pxAddress,
                          socklen_t xAddressLength )
{
	BaseType_t xRet;
	FreeRTOS_TSN_Socket_t * pxSocket = ( FreeRTOS_TSN_Socket_t * ) xSocket;
	FreeRTOS_Socket_t * pxBaseSocket = ( FreeRTOS_Socket_t * ) pxSocket->xBaseSocket;
	/* the socket will be placed in the udp socket
	 * list together with plus-tcp sockets */
	xRet = FreeRTOS_bind( pxSocket->xBaseSocket, pxAddress, xAddressLength );

	if( xRet == 0 )
	{
		tsnsocketSET_SOCKET_PORT( pxSocket, FreeRTOS_htons( pxBaseSocket->usLocalPort ) );
		vListInsertEnd( &xTSNBoundUDPSocketList, &( pxSocket->xBoundSocketListItem ) );
	}

	return xRet;
}

BaseType_t FreeRTOS_TSN_closesocket( TSNSocket_t xSocket )
{
	FreeRTOS_TSN_Socket_t * pxSocket = ( FreeRTOS_TSN_Socket_t * ) xSocket;
	//FreeRTOS_Socket_t * pxBaseSocket = ( FreeRTOS_Socket_t * ) pxSocket->xBaseSocket;

	if( xSocket == FREERTOS_TSN_INVALID_SOCKET )
	{
		return 0;
	}

	( void ) uxListRemove( &( pxSocket->xBoundSocketListItem ) );

	return FreeRTOS_closesocket( pxSocket->xBaseSocket );
}

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
		FreeRTOS_debug_printf( ("sendto: invalid destination address\n") );
		return -pdFREERTOS_ERRNO_EINVAL;
	}

	if( ! tsnsocketSOCKET_IS_BOUND( pxBaseSocket ) )
	{
		FreeRTOS_debug_printf( ("sendto: socket is not bound\n") );
		return -pdFREERTOS_ERRNO_EBADF;
	}

    switch( pxDestinationAddress->sin_family )
    {
        #if ( ipconfigUSE_IPv6 != 0 )
            case FREERTOS_AF_INET6:
				uxPayloadOffset = ipSIZE_OF_ETH_HEADER + ipSIZE_OF_IPv6_HEADER + ipSIZE_OF_UDP_HEADER;
                #if ( tsnconfigSOCKET_INSERTS_VLAN_TAGS != tsnconfigDISABLE )
				uxPayloadOffset += pxSocket->ucVLANTagsCount * sizeof( struct xVLAN_TAG )
				#endif
				uxMaxPayloadLength = ipconfigNETWORK_MTU - ( ipSIZE_OF_IPv6_HEADER + ipSIZE_OF_UDP_HEADER );
				pxBuf = pxGetNetworkBufferWithDescriptor( uxTotalDataLength + uxPayloadOffset, pxBaseSocket->xSendBlockTime );
				if( pxBuf == NULL )
				{
					FreeRTOS_debug_printf( ("sendto: couldn't acquire network buffer\n") );
					return -pdFREERTOS_ERRNO_EAGAIN;
				}
				pxBuf->pxEndPoint = pxBaseSocket->pxEndPoint;
				pxBuf->usPort = pxDestinationAddress->sin_port;
				pxBuf->usBoundPort = ( uint16_t ) tsnsocketGET_SOCKET_PORT( pxBaseSocket );

				( void ) memcpy( pxBuf->xIPAddress.xIP_IPv6.ucBytes, pxDestinationAddress->sin_address.xIP_IPv6.ucBytes, ipSIZE_OF_IPv6_ADDRESS );
				//prvPrepareBufferUDPv6( pxSocket, pxBuf, xFlags, pxDestinationAddress, xDestinationAddressLength );
                break;
        #endif /* ( ipconfigUSE_IPv6 != 0 ) */

        #if ( ipconfigUSE_IPv4 != 0 )
            case FREERTOS_AF_INET4:
				uxPayloadOffset = ipSIZE_OF_ETH_HEADER + ipSIZE_OF_IPv4_HEADER + ipSIZE_OF_UDP_HEADER;
                #if ( tsnconfigSOCKET_INSERTS_VLAN_TAGS != tsnconfigDISABLE )
				uxPayloadOffset += pxSocket->ucVLANTagsCount * sizeof( struct xVLAN_TAG )
				#endif
				uxMaxPayloadLength = ipconfigNETWORK_MTU - ( ipSIZE_OF_IPv4_HEADER + ipSIZE_OF_UDP_HEADER );
				pxBuf = pxGetNetworkBufferWithDescriptor( uxTotalDataLength + uxPayloadOffset, pxBaseSocket->xSendBlockTime );
				if( pxBuf == NULL )
				{
					FreeRTOS_debug_printf( ("sendto: couldn't acquire network buffer\n") );
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
			FreeRTOS_debug_printf( ("sendto: invalid sin familyl\n") );
            return -pdFREERTOS_ERRNO_EINVAL;
	}

	if( uxTotalDataLength > uxMaxPayloadLength )
	{
		FreeRTOS_debug_printf( ("sendto: payload is too large\n") );
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
			FreeRTOS_debug_printf( ("sendto: cannot insert into network queues\n") );
			vReleaseNetworkBufferAndDescriptor( pxBuf );
			return -pdFREERTOS_ERRNO_EAGAIN;
		}
	}
	else
	{
		FreeRTOS_debug_printf( ("sendto: timeout occurred\n") );
		vReleaseNetworkBufferAndDescriptor( pxBuf );
		return -pdFREERTOS_ERRNO_ETIMEDOUT;
	}
}

int32_t FreeRTOS_TSN_recvmsg( TSNSocket_t xSocket, struct msghdr * pxMsghUser, BaseType_t xFlags )
{
	FreeRTOS_TSN_Socket_t * pxSocket = ( FreeRTOS_TSN_Socket_t * ) xSocket;
	FreeRTOS_Socket_t * pxBaseSocket = ( FreeRTOS_Socket_t * ) pxSocket->xBaseSocket;

    if( pxSocket == NULL || pxSocket == FREERTOS_TSN_INVALID_SOCKET )
    {
		return -pdFREERTOS_ERRNO_EINVAL;
    }
    if( !tsnsocketSOCKET_IS_BOUND( pxBaseSocket ) )
    {
		return -pdFREERTOS_ERRNO_EINVAL;
    }

	if( pxMsghUser == NULL )
	{
		return -pdFREERTOS_ERRNO_EINVAL;
	}

	BaseType_t xTimed = pdFALSE;
    TickType_t xRemainingTime = pxBaseSocket->xReceiveBlockTime;
    BaseType_t lPacketCount;
    TimeOut_t xTimeOut;
    NetworkBufferDescriptor_t * pxNetworkBuffer;
	struct msghdr * pxMsgh;
	size_t uxPayloadLen = 0;
	size_t uxLen;

    lPacketCount = ( BaseType_t ) listCURRENT_LIST_LENGTH( &( pxBaseSocket->u.xUDP.xWaitingPacketsList ) );

	if( xFlags & FREERTOS_MSG_ERRQUEUE )
	{
		if( xQueueReceive( pxSocket->xErrQueue, &pxMsgh, 0) == pdFALSE )
		{
			pxMsgh = NULL;
		}
	}
	else
	{
		/* Note: this part is a copy-paste from FreeRTOS_Sockets.c */

		while( lPacketCount == 0 )
		{
			if( xTimed == pdFALSE )
			{
				/* Check to see if the socket is non blocking on the first
				* iteration.  */
				if( xRemainingTime == ( TickType_t ) 0 )
				{
					break;
				}

				if( ( ( ( UBaseType_t ) xFlags ) & ( ( UBaseType_t ) FREERTOS_MSG_DONTWAIT ) ) != 0U )
				{
					break;
				}

				/* To ensure this part only executes once. */
				xTimed = pdTRUE;

				/* Fetch the current time. */
				vTaskSetTimeOutState( &xTimeOut );
			}

			( void ) xEventGroupWaitBits( pxBaseSocket->xEventGroup, ( ( EventBits_t ) eSOCKET_RECEIVE ) | ( ( EventBits_t ) eSOCKET_INTR ),
											pdTRUE /*xClearOnExit*/, pdFALSE /*xWaitAllBits*/, xRemainingTime );

			lPacketCount = ( BaseType_t ) listCURRENT_LIST_LENGTH( &( pxBaseSocket->u.xUDP.xWaitingPacketsList ) );

			if( lPacketCount != 0 )
			{
				break;
			}

			/* Has the timeout been reached ? */
			if( xTaskCheckForTimeOut( &xTimeOut, &xRemainingTime ) != pdFALSE )
			{
				break;
			}
		} /* while( lPacketCount == 0 ) */

		if( lPacketCount > 0 )
		{
			vTaskSuspendAll();
			{
				/* The owner of the list item is the network buffer. */
				pxNetworkBuffer = ( ( NetworkBufferDescriptor_t * ) listGET_OWNER_OF_HEAD_ENTRY( &( pxBaseSocket->u.xUDP.xWaitingPacketsList ) ) );

				if( ( ( UBaseType_t ) xFlags & ( UBaseType_t ) FREERTOS_MSG_PEEK ) == 0U )
				{
					/* Remove the network buffer from the list of buffers waiting to
					* be processed by the socket. */
					( void ) uxListRemove( &( pxNetworkBuffer->xBufferListItem ) );
				}
			}
			( void ) xTaskResumeAll();
		}

		pxMsgh = ( struct msghdr *) pxNetworkBuffer->pucEthernetBuffer;

	}

	if( pxMsgh == NULL )
	{
		return -pdFREERTOS_ERRNO_EWOULDBLOCK;
	}


	if( pxMsgh->msg_name != NULL && pxMsghUser->msg_name != NULL )
	{
		uxLen = configMIN( pxMsghUser->msg_namelen, pxMsgh->msg_namelen );
		memcpy( pxMsghUser->msg_name, pxMsgh->msg_name, uxLen );
		pxMsghUser->msg_namelen = uxLen;
	}
	else
	{
		pxMsghUser->msg_namelen = 0;
	}

	if( pxMsgh->msg_iov != NULL && pxMsghUser->msg_iov != NULL )
	{
		pxMsghUser->msg_iovlen = configMIN( pxMsghUser->msg_iovlen, pxMsgh->msg_iovlen );

		/* here len is the number of elements in the array */
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

	if( pxMsghUser->msg_control != NULL && pxMsgh->msg_control != NULL )
	{
		/* here len is the number of elements in the array */
		uxLen = configMIN( pxMsghUser->msg_controllen, pxMsgh->msg_controllen );
		memcpy( pxMsghUser->msg_control, pxMsgh->msg_control, uxLen );
		pxMsghUser->msg_controllen = uxLen;
	}
	else
	{
		pxMsghUser->msg_controllen = 0;
	}

	pxMsghUser->msg_flags = pxMsgh->msg_flags;

	vAncillaryMsgFreeAll( pxMsgh );

	return uxPayloadLen;
}

int32_t FreeRTOS_TSN_recvfrom( TSNSocket_t xSocket,
                           void * pvBuffer,
                           size_t uxBufferLength,
                           BaseType_t xFlags,
                           struct freertos_sockaddr * pxSourceAddress,
                           socklen_t * pxSourceAddressLength )
{
	struct msghdr xMsghdr;
	struct iovec xIovec;
	xMsghdr.msg_name = pxSourceAddress;
	xMsghdr.msg_namelen = sizeof( struct freertos_sockaddr );

	/* errors are not copied from the original msg */
	xMsghdr.msg_control = NULL;
	xMsghdr.msg_controllen = 0;

	xIovec.iov_base = pvBuffer;
	xIovec.iov_len = uxBufferLength;

	xMsghdr.msg_iov = &xIovec;
	xMsghdr.msg_iovlen = 1;

	/* use instead recvmsg() to receive on errqueue */
	xFlags &= ~FREERTOS_MSG_ERRQUEUE;

	return FreeRTOS_TSN_recvmsg( xSocket, &xMsghdr, xFlags );
}