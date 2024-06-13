#include "FreeRTOS.h"

#include "FreeRTOS_IP.h"

#include "FreeRTOS_TSN_VLANTags.h"
#include "FreeRTOS_TSN_DS.h"

/**
 * @brief Retrieves the IP version and offset from the given network buffer.
 *
 * This function extracts the IP version and offset from the Ethernet header of the network buffer.
 *
 * @param[in] pxBuf The network buffer descriptor.
 * @param[out] pusIPVersion Pointer to store the IP version.
 * @param[out] pulOffset Pointer to store the offset.
 */
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

/**
 * @brief Retrieves the DiffServ class from the given network buffer.
 *
 * This function extracts the DiffServ class from the IP header of the network buffer based on the IP version.
 *
 * @param[in] pxBuf The network buffer descriptor.
 * @return The DiffServ class value.
 */
uint8_t ucDSClassGet( NetworkBufferDescriptor_t * pxBuf )
{
	size_t ulIPHeaderOffset;
	uint16_t usEtherType;

	prvGetIPVersionAndOffset( pxBuf, &usEtherType, &ulIPHeaderOffset );

	switch( usEtherType )
	{
		case ipIPv4_FRAME_TYPE:
			IPHeader_t * pxIPHeader = ( IPHeader_t * ) &pxBuf->pucEthernetBuffer[ ulIPHeaderOffset ];
			return diffservGET_DSCLASS_IPv4( pxIPHeader );

		case ipIPv6_FRAME_TYPE:
			IPHeader_IPv6_t * pxIPv6Header = ( IPHeader_IPv6_t * ) &pxBuf->pucEthernetBuffer[ ulIPHeaderOffset ];
			return diffservGET_DSCLASS_IPv6( pxIPv6Header );
		default:
			return ( uint8_t ) ~0;
	}
}

/**
 * @brief Sets the DiffServ class for the given network buffer.
 *
 * This function sets the DiffServ class in the IP header of the network buffer based on the IP version.
 *
 * @param[in] pxBuf The network buffer descriptor.
 * @param[in] ucValue The DiffServ class value to set.
 * @return pdPASS if the DiffServ class was set successfully, pdFAIL otherwise.
 */
BaseType_t xDSClassSet( NetworkBufferDescriptor_t * pxBuf, uint8_t ucValue )
{
	size_t ulIPHeaderOffset;
	uint16_t usEtherType;

	prvGetIPVersionAndOffset( pxBuf, &usEtherType, &ulIPHeaderOffset );
	
	switch( usEtherType )
	{
		case ipIPv4_FRAME_TYPE:
			IPHeader_t * pxIPHeader = ( IPHeader_t * ) &pxBuf->pucEthernetBuffer[ ulIPHeaderOffset ];
			diffservSET_DSCLASS_IPv4( pxIPHeader, ucValue );
			return pdPASS;

		case ipIPv6_FRAME_TYPE:
			IPHeader_IPv6_t * pxIPv6Header = ( IPHeader_IPv6_t * ) &pxBuf->pucEthernetBuffer[ ulIPHeaderOffset ];
			diffservSET_DSCLASS_IPv6( pxIPv6Header, ucValue );
			return pdPASS;

		default:
			return pdFAIL;
	}
}
