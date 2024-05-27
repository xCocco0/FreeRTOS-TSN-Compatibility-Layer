
#include <string.h>

#include "FreeRTOS.h"

#include "FreeRTOS_IP.h"

#include "FreeRTOS_TSN_VLANTags.h"

#define ETHERTYPE_TPID 0x0081 /* EtherType for VLAN 802.1Q Header (in little endian) */
#define ETHERTYPE_OFFSET 12

#define PCP_FROM_TCI( x ) ( ( x & 0xE000 ) >> 13 )
#define DEI_FROM_TCI( x ) ( ( x & 0x1000 ) >> 12 )
#define VID_FROM_TCI( x ) ( ( x & 0x0FFF ) )


BaseType_t xVLANTagGetPCP( NetworkBufferDescriptor_t * pxBuf )
{
	const struct xVLAN_TAG * pxVLANField = ( const struct xVLAN_TAG * ) &( pxBuf->pucEthernetBuffer[ ETHERTYPE_OFFSET ] );
	return PCP_FROM_TCI( pxVLANField->usTCI );
}

BaseType_t xVLANTagGetDEI( NetworkBufferDescriptor_t * pxBuf )
{
	const struct xVLAN_TAG * pxVLANField = ( const struct xVLAN_TAG * ) &( pxBuf->pucEthernetBuffer[ ETHERTYPE_OFFSET ] );
	return DEI_FROM_TCI( pxVLANField->usTCI );
}

BaseType_t xVLANTagGetVID( NetworkBufferDescriptor_t * pxBuf )
{
	const struct xVLAN_TAG * pxVLANField = ( const struct xVLAN_TAG * ) &( pxBuf->pucEthernetBuffer[ ETHERTYPE_OFFSET ] );
	return VID_FROM_TCI( pxVLANField->usTCI );
}

BaseType_t xVLANTagCheckTPID( NetworkBufferDescriptor_t * pxBuf )
{
	const struct xVLAN_TAG * pxVLANField = ( const struct xVLAN_TAG * ) &( pxBuf->pucEthernetBuffer[ ETHERTYPE_OFFSET ] );
	return pxVLANField->usTPID == ETHERTYPE_TPID;
}

BaseType_t xVLANTagCheckClass( NetworkBufferDescriptor_t * pxBuf, BaseType_t xClass )
{
	return xVLANTagCheckTPID( pxBuf ) && ( xVLANTagGetPCP( pxBuf ) == xClass );
}
