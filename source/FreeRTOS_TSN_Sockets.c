
#include "FreeRTOS.h"

#include "FreeRTOS_IP.h"
#include "FreeRTOS_IP_Private.h"
#include "FreeRTOS_ARP.h"

#include "FreeRTOS_TSN_Sockets.h"
#include "FreeRTOS_TSN_NetworkScheduler.h"
#include "FreeRTOS_TSN_VLANTags.h"

/* private definitions from FreeRTOS_Sockets.c */
#define tsnsocketSET_SOCKET_PORT( pxSocket, usPort )    listSET_LIST_ITEM_VALUE( ( &( ( pxSocket )->xBoundSocketListItem ) ), ( usPort ) )
#define tsnsocketGET_SOCKET_PORT( pxSocket )            listGET_LIST_ITEM_VALUE( ( &( ( pxSocket )->xBoundSocketListItem ) ) )
#define tsnsocketSOCKET_IS_BOUND( pxSocket )            ( listLIST_ITEM_CONTAINER( &( pxSocket )->xBoundSocketListItem ) != NULL )

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
	size_t uxVLANOffset = sizeof( struct xVLAN_TAG ) * pxSocket->ucVLANTagsCount;
	size_t uxPayloadSize = pxBuf->xDataLength - sizeof( UDPPacket_t );
	NetworkQueueItem_t xQueueItem;

	pxEthernetHeader = ( EthernetHeader_t * ) pxBuf->pucEthernetBuffer;

	/* reset because it can happen that random garbage can find a match
	 * in the ARP tables */
	memset( &pxEthernetHeader->xDestinationAddress, '\0', sizeof( MACAddress_t ) );

	eReturned = eARPGetCacheEntry( &( pxBuf->xIPAddress.ulIP_IPv4 ), &( pxEthernetHeader->xDestinationAddress ), &( pxEndPoint ) );
	
	if( eReturned != eARPCacheHit )
	{
		return pdFAIL;
	}
	
	pxBuf->pxEndPoint = pxEndPoint;

	( void ) memcpy( pxEthernetHeader->xSourceAddress.ucBytes, pxEndPoint->xMACAddress.ucBytes, ( size_t ) ipMAC_ADDRESS_LENGTH_BYTES );

	switch( pxSocket->ucVLANTagsCount )
	{
		case 0:
			/* can reuse previous struct */
			pxEthernetHeader->usFrameType = ipIPv4_FRAME_TYPE;
			break;

		case 1:
			TaggedEthernetHeader_t * pxTEthHeader = ( TaggedEthernetHeader_t * ) pxEthernetHeader;
			pxTEthHeader->xVLANTag.usTPID = FreeRTOS_htons( vlantagTPID_DEFAULT );
			pxTEthHeader->xVLANTag.usTCI = FreeRTOS_htons( pxSocket->ucVLANCTagTCI );
			pxTEthHeader->usFrameType = ipIPv4_FRAME_TYPE;
			break;

		case 2:
			DoubleTaggedEthernetHeader_t * pxDTEthHeader = ( DoubleTaggedEthernetHeader_t * ) pxEthernetHeader;
			pxDTEthHeader->xVLANSTag.usTPID = FreeRTOS_htons( vlantagTPID_DOUBLE_TAG );
			pxDTEthHeader->xVLANSTag.usTCI = FreeRTOS_htons( pxSocket->ucVLANSTagTCI );
			pxDTEthHeader->xVLANCTag.usTPID = FreeRTOS_htons( vlantagTPID_DEFAULT );
			pxDTEthHeader->xVLANCTag.usTCI = FreeRTOS_htons( pxSocket->ucVLANCTagTCI );
			pxDTEthHeader->usFrameType = ipIPv4_FRAME_TYPE;
			break;

		default:
			return pdFAIL;
	}

	pxIPHeader = ( IPHeader_t * ) &pxBuf[ ipSIZE_OF_ETH_HEADER + uxVLANOffset ];

	pxIPHeader->ucVersionHeaderLength = ipIPV4_VERSION_HEADER_LENGTH_MIN;
	pxIPHeader->ucDifferentiatedServicesCode = pxSocket->ucDSClass;
	pxIPHeader->usLength = uxPayloadSize + sizeof( IPHeader_t ) + sizeof( UDPHeader_t );
	pxIPHeader->usLength = FreeRTOS_htons( pxIPHeader->usLength );
	pxIPHeader->usIdentification = 0;
	pxIPHeader->usFragmentOffset = 0;
	pxIPHeader->ucTimeToLive = ipconfigUDP_TIME_TO_LIVE;
	pxIPHeader->ucProtocol = ipPROTOCOL_UDP;
	pxIPHeader->ulSourceIPAddress = pxBuf->pxEndPoint->ipv4_settings.ulIPAddress;
	pxIPHeader->ulDestinationIPAddress = pxBuf->xIPAddress.ulIP_IPv4;
	
	pxUDPHeader = ( UDPHeader_t * ) &pxBuf[ ipSIZE_OF_ETH_HEADER + uxVLANOffset + ipSIZE_OF_IPv4_HEADER ];

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

	xQueueItem.eEventType = eNetworkTxEvent;
	xQueueItem.pvData = ( void * ) pxBuf;
	xQueueItem.xReleaseAfterSend = pdTRUE;

	return xNetworkQueueInsertPacketByFilter( &xQueueItem );
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
	uint32_t ulOptionValue = ( uint32_t ) pvOptionValue;

    if( xSocketValid( pxSocket->xBaseSocket ) == pdTRUE )
	{
		switch( lOptionName )
		{
			case FREERTOS_SO_VLAN_CTAG:
				if( ulOptionValue < 8 )
				{
					pxSocket->ucVLANCTagTCI = ulOptionValue;
					if( pxSocket->ucVLANTagsCount == 0 )
					{
						pxSocket->ucVLANTagsCount = 1;
					}
					xReturn = 0;
				}
				break;

			case FREERTOS_SO_VLAN_STAG:
				if( ulOptionValue < 8 )
				{
					pxSocket->ucVLANSTagTCI = ulOptionValue;
					pxSocket->ucVLANTagsCount = 2;
					xReturn = 0;
				}
				break;

			case FREERTOS_SO_VLAN_TAG_RST:
				pxSocket->ucVLANTagsCount = 0;
				xReturn = 0;
				break;

			case FREERTOS_SO_DS_CLASS:
				if( ulOptionValue < 64 )
				{
					pxSocket->ucDSClass = ulOptionValue;
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
	FreeRTOS_TSN_Socket_t * pxSocket = ( FreeRTOS_TSN_Socket_t * ) xSocket;
	/* the socket will be placed in the udp socket
	 * list together with plus-tcp sockets */
	return FreeRTOS_bind( pxSocket->xBaseSocket, pxAddress, xAddressLength );
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
		return -pdFREERTOS_ERRNO_EINVAL;
	}

	if( ! tsnsocketSOCKET_IS_BOUND( pxBaseSocket ) )
	{
		return -pdFREERTOS_ERRNO_EBADF;
	}

	pxBuf = pxGetNetworkBufferWithDescriptor( uxTotalDataLength, pxBaseSocket->xSendBlockTime );
	
	if( pxBuf == NULL )
	{
		return -pdFREERTOS_ERRNO_EAGAIN;
	}

	pxBuf->pxEndPoint = pxBaseSocket->pxEndPoint;
    pxBuf->usPort = pxDestinationAddress->sin_port;
    pxBuf->usBoundPort = ( uint16_t ) tsnsocketGET_SOCKET_PORT( pxBaseSocket );

    switch( pxDestinationAddress->sin_family )
    {
        #if ( ipconfigUSE_IPv6 != 0 )
            case FREERTOS_AF_INET6:
				uxPayloadOffset = ipSIZE_OF_ETH_HEADER + pxSocket->ucVLANTagsCount * sizeof( struct xVLAN_TAG ) + ipSIZE_OF_IPv6_HEADER + ipSIZE_OF_UDP_HEADER;
                uxMaxPayloadLength = ipconfigNETWORK_MTU - ( ipSIZE_OF_IPv6_HEADER + ipSIZE_OF_UDP_HEADER );
				pxBuf = pxGetNetworkBufferWithDescriptor( uxTotalDataLength + uxPayloadOffset, pxBaseSocket->xSendBlockTime );

				( void ) memcpy( pxBuf->xIPAddress.xIP_IPv6.ucBytes, pxDestinationAddress->sin_address.xIP_IPv6.ucBytes, ipSIZE_OF_IPv6_ADDRESS );
				//prvPrepareBufferUDPv6( pxSocket, pxBuf, xFlags, pxDestinationAddress, xDestinationAddressLength );
                break;
        #endif /* ( ipconfigUSE_IPv6 != 0 ) */

        #if ( ipconfigUSE_IPv4 != 0 )
            case FREERTOS_AF_INET4:
				uxPayloadOffset = ipSIZE_OF_ETH_HEADER + pxSocket->ucVLANTagsCount * sizeof( struct xVLAN_TAG ) + ipSIZE_OF_IPv4_HEADER + ipSIZE_OF_UDP_HEADER;
                uxMaxPayloadLength = ipconfigNETWORK_MTU - ( ipSIZE_OF_IPv4_HEADER + ipSIZE_OF_UDP_HEADER );
				pxBuf = pxGetNetworkBufferWithDescriptor( uxTotalDataLength + uxPayloadOffset, pxBaseSocket->xSendBlockTime );

				pxBuf->xIPAddress.ulIP_IPv4 = pxDestinationAddress->sin_address.ulIP_IPv4;

				if( prvPrepareBufferUDPv4( pxSocket, pxBuf, xFlags, pxDestinationAddress, xDestinationAddressLength ) != pdPASS )
				{
					vReleaseNetworkBufferAndDescriptor( pxBuf );
					return -pdFREERTOS_ERRNO_EAGAIN;
				}
                break;
        #endif /* ( ipconfigUSE_IPv4 != 0 ) */

        default:
			vReleaseNetworkBufferAndDescriptor( pxBuf );
            return -pdFREERTOS_ERRNO_EINVAL;
	}

	if( uxTotalDataLength > uxMaxPayloadLength )
	{
		vReleaseNetworkBufferAndDescriptor( pxBuf );
		return -pdFREERTOS_ERRNO_EINVAL;
	}
	
	memcpy( &pxBuf->pucEthernetBuffer[ uxPayloadOffset ], pvBuffer, uxTotalDataLength );

	xEvent.eEventType = eNetworkTxEvent;
	xEvent.pvData = ( void * ) pxBuf;
	xEvent.xReleaseAfterSend = pdTRUE;

	if( xNetworkQueueInsertPacketByFilter( &xEvent ) == pdPASS )
	{
		if( xTaskCheckForTimeOut( &xTimeOut, &xRemainingTime ) != pdFALSE )
		{
			/* not implemented yet */
		}

		return uxTotalDataLength;
	}
	else
	{
		vReleaseNetworkBufferAndDescriptor( pxBuf );
		return -pdFREERTOS_ERRNO_EAGAIN;
	}
}

int32_t FreeRTOS_TSN_recvfrom( TSNSocket_t xSocket,
                           void * pvBuffer,
                           size_t uxBufferLength,
                           BaseType_t xFlags,
                           struct freertos_sockaddr * pxSourceAddress,
                           socklen_t * pxSourceAddressLength )
{
}


