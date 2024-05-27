
#include "FreeRTOS.h"

#include "FreeRTOS_IP.h"

#include "FreeRTOS_TSN_VLANTags.h"
#include "FreeRTOS_TSN_DS.h"

#define diffservGET_DSCLASS_FROM_IPv4( pxIPHeader ) \
	( pxIPHeader->ucDifferentiatedServicesCode )

#define diffservGET_DSCLASS_FROM_IPv6( pxIPHeader ) \
	( ( ( pxIPHeader->ucVersionTrafficClass & 0xF ) << 2 ) | ( ( pxIPHeader->ucTrafficClassFlow & 0xC0 ) >> 6 ) )

#define diffservSET_DSCLASS_FROM_IPv4( pxIPHeader, ucValue ) do { \
	pxIPHeader->ucDifferentiatedServicesCode = ucValue; \
	} while( ipFALSE_BOOL )

#define diffservSET_DSCLASS_FROM_IPv6( pxIPHeader, ucValue ) do { \
	pxIPHeader->ucVersionTrafficClass &= 0xF0; \
	pxIPHeader->ucVersionTrafficClass |= ( ( ucValue & 0x3C ) >> 2 ); \
	pxIPHeader->ucTrafficClassFlow &= 0x3F; \
	pxIPHeader->ucTrafficClassFlow |= ( ( ucValue & 0x3 ) << 6 ); \
	} while( ipFALSE_BOOL )
	

void prvGetIPVersionAndOffset( NetworkBufferDescriptor_t * pxBuf, uint16_t * pusIPVersion, size_t * pulOffset )
{
	EthernetHeader_t * pxEthHead = ( EthernetHeader_t * ) pxBuf->pucEthernetBuffer;
	uint16_t usEthType = FreeRTOS_ntohs( pxEthHead->usFrameType );
	*pulOffset = ipSIZE_OF_ETH_HEADER;

	if( usEthType == vlantagTPID_DOUBLE_TAG )
	{
		*pulOffset += 2 * sizeof( struct xVLAN_TAG );
		DoubleTaggedEthernetHeader_t * pxDTEthHead = ( DoubleTaggedEthernetHeader_t * ) pxBuf->pucEthernetBuffer;
		*pusIPVersion = pxDTEthHead->usFrameType;
	}
	else if( usEthType == vlantagTPID_DEFAULT )
	{
		*pulOffset += sizeof( struct xVLAN_TAG );
		TaggedEthernetHeader_t * pxTEthHead = ( TaggedEthernetHeader_t * ) pxBuf->pucEthernetBuffer;
		*pusIPVersion = pxTEthHead->usFrameType;
	}
}

uint8_t ucDSClassGet( NetworkBufferDescriptor_t * pxBuf )
{
	size_t ulIPHeaderOffset;
	uint16_t usEtherType;

	prvGetIPVersionAndOffset( pxBuf, &usEtherType, &ulIPHeaderOffset );

	switch( usEtherType )
	{
		case ipIPv4_FRAME_TYPE:
			IPHeader_t * pxIPHeader = ( IPHeader_t * ) &pxBuf->pucEthernetBuffer[ ulIPHeaderOffset ];
			return diffservGET_DSCLASS_FROM_IPv4( pxIPHeader );

		case ipIPv6_FRAME_TYPE:
			IPHeader_IPv6_t * pxIPv6Header = ( IPHeader_IPv6_t * ) &pxBuf->pucEthernetBuffer[ ulIPHeaderOffset ];
			return diffservGET_DSCLASS_FROM_IPv6( pxIPv6Header );
		default:
			return ( uint8_t ) ~0;
	}
}

BaseType_t xDSClassSet( NetworkBufferDescriptor_t * pxBuf, uint8_t ucValue )
{
	size_t ulIPHeaderOffset;
	uint16_t usEtherType;

	prvGetIPVersionAndOffset( pxBuf, &usEtherType, &ulIPHeaderOffset );
	
	switch( usEtherType )
	{
		case ipIPv4_FRAME_TYPE:
			IPHeader_t * pxIPHeader = ( IPHeader_t * ) &pxBuf->pucEthernetBuffer[ ulIPHeaderOffset ];
			diffservSET_DSCLASS_FROM_IPv4( pxIPHeader, ucValue );
			return pdPASS;

		case ipIPv6_FRAME_TYPE:
			IPHeader_IPv6_t * pxIPv6Header = ( IPHeader_IPv6_t * ) &pxBuf->pucEthernetBuffer[ ulIPHeaderOffset ];
			diffservSET_DSCLASS_FROM_IPv6( pxIPv6Header, ucValue );
			return pdPASS;

		default:
			return pdFAIL;
	}
}
