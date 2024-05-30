
#include <string.h>

#include "FreeRTOS.h"

#include "FreeRTOS_IP.h"

#include "FreeRTOS_TSN_VLANTags.h"


uint8_t prvGetNumTags( NetworkBufferDescriptor_t * pxBuf )
{
	EthernetHeader_t * pxEthHead = ( EthernetHeader_t * ) pxBuf->pucEthernetBuffer;
	uint16_t usEthType = FreeRTOS_ntohs( pxEthHead->usFrameType );

	if( usEthType == vlantagTPID_DOUBLE_TAG )
	{
		return ( uint8_t ) 1;
	}
	else if( usEthType == vlantagTPID_DEFAULT )
	{
		return ( uint8_t ) 2;
	}
	else
	{
		return ( uint8_t ) 0;
	}
}

struct xVLAN_TAG * prvGetVLANSTag( NetworkBufferDescriptor_t * pxBuf, uint8_t ucNumTags )
{
	size_t uxOffset;

	switch( ucNumTags )
	{
		case 2:
			uxOffset = offsetof( EthernetHeader_t, usFrameType );
		default:
			return NULL;
	}

	return ( struct xVLAN_TAG * ) &pxBuf->pucEthernetBuffer[ uxOffset ];
}

struct xVLAN_TAG * prvGetVLANCTag( NetworkBufferDescriptor_t * pxBuf, uint8_t ucNumTags )
{
	size_t uxOffset;

	switch( ucNumTags )
	{
		case 1:
			uxOffset = offsetof( EthernetHeader_t, usFrameType );
		case 2:
			uxOffset = offsetof( EthernetHeader_t, usFrameType ) + sizeof( struct xVLAN_TAG );
		default:
			return NULL;
	}

	return ( struct xVLAN_TAG * ) &pxBuf->pucEthernetBuffer[ uxOffset ];
}


struct xVLAN_TAG * prvPrepareAndGetVLANCTag( NetworkBufferDescriptor_t * pxBuf )
{
	struct xVLAN_TAG * pxVLANField;
	uint8_t ucNumTags = prvGetNumTags( pxBuf );
	
	switch( ucNumTags )
	{
		case 0:
			pxVLANField = prvGetVLANCTag( pxBuf, 1 );
			pxVLANField->usTPID = FreeRTOS_htons( vlantagTPID_DEFAULT );
			pxVLANField->usTCI = 0;
			break;
		case 1:
		case 2:
			pxVLANField = prvGetVLANCTag( pxBuf, 2 );
			break;
		default:
			return NULL;
	}
	
	return pxVLANField;
}

struct xVLAN_TAG * prvPrepareAndGetVLANSTag( NetworkBufferDescriptor_t * pxBuf )
{
	struct xVLAN_TAG * pxVLANFieldC, * pxVLANFieldS;
	uint8_t ucNumTags = prvGetNumTags( pxBuf );
	
	switch( ucNumTags )
	{
		case 0:
			pxVLANFieldS = prvGetVLANSTag( pxBuf, 2 );
			pxVLANFieldS->usTPID = FreeRTOS_htons( vlantagTPID_DOUBLE_TAG );
			pxVLANFieldS->usTCI = 0;
			pxVLANFieldC = prvGetVLANCTag( pxBuf, 2 );
			pxVLANFieldC->usTPID = FreeRTOS_htons( vlantagTPID_DEFAULT );
			pxVLANFieldC->usTCI = 0;
			break;
		case 1:
			/* move single tag to customer tag space */
			pxVLANFieldS = prvGetVLANSTag( pxBuf, 2 );
			pxVLANFieldC = prvGetVLANCTag( pxBuf, 2 );

			pxVLANFieldC->usTPID = FreeRTOS_htons( vlantagTPID_DEFAULT );
			pxVLANFieldC->usTCI = pxVLANFieldS->usTCI;
			pxVLANFieldS->usTPID = FreeRTOS_htons( vlantagTPID_DOUBLE_TAG );
			pxVLANFieldS->usTCI = 0;
		case 2:
			pxVLANFieldC = prvGetVLANCTag( pxBuf, 2 );
			break;
		default:
			return NULL;
	}

	return pxVLANFieldC;
}


BaseType_t xVLANSTagGetPCP( NetworkBufferDescriptor_t * pxBuf )
{
	uint8_t ucNumTags = prvGetNumTags( pxBuf );
	const struct xVLAN_TAG * pxVLANField = prvGetVLANSTag( pxBuf, ucNumTags );
	if( pxVLANField != NULL )
	{
		return vlantagGET_PCP_FROM_TCI( pxVLANField->usTCI );
	}

	return ~0;
}

BaseType_t xVLANSTagGetDEI( NetworkBufferDescriptor_t * pxBuf )
{
	uint8_t ucNumTags = prvGetNumTags( pxBuf );
	const struct xVLAN_TAG * pxVLANField = prvGetVLANSTag( pxBuf, ucNumTags );
	if( pxVLANField != NULL )
	{
		return vlantagGET_DEI_FROM_TCI( pxVLANField->usTCI );
	}

	return ~0;
}

BaseType_t xVLANSTagGetVID( NetworkBufferDescriptor_t * pxBuf )
{
	uint8_t ucNumTags = prvGetNumTags( pxBuf );
	const struct xVLAN_TAG * pxVLANField = prvGetVLANSTag( pxBuf, ucNumTags );
	if( pxVLANField != NULL )
	{
		return vlantagGET_VID_FROM_TCI( pxVLANField->usTCI );
	}

	return ~0;
}

BaseType_t xVLANSTagCheckClass( NetworkBufferDescriptor_t * pxBuf, BaseType_t xClass )
{
	BaseType_t xFoundClass = xVLANSTagGetPCP( pxBuf );
	return ( ( xFoundClass != ~0 ) && ( xFoundClass == xClass ) ) ? pdTRUE : pdFALSE;
}


BaseType_t xVLANCTagGetPCP( NetworkBufferDescriptor_t * pxBuf )
{
	uint8_t ucNumTags = prvGetNumTags( pxBuf );
	const struct xVLAN_TAG * pxVLANField = prvGetVLANCTag( pxBuf, ucNumTags );
	if( pxVLANField != NULL )
	{
		return vlantagGET_PCP_FROM_TCI( pxVLANField->usTCI );
	}

	return ~0;
}

BaseType_t xVLANCTagGetDEI( NetworkBufferDescriptor_t * pxBuf )
{
	uint8_t ucNumTags = prvGetNumTags( pxBuf );
	const struct xVLAN_TAG * pxVLANField = prvGetVLANCTag( pxBuf, ucNumTags );
	if( pxVLANField != NULL )
	{
		return vlantagGET_DEI_FROM_TCI( pxVLANField->usTCI );
	}

	return ~0;
}

BaseType_t xVLANCTagGetVID( NetworkBufferDescriptor_t * pxBuf )
{
	uint8_t ucNumTags = prvGetNumTags( pxBuf );
	const struct xVLAN_TAG * pxVLANField = prvGetVLANCTag( pxBuf, ucNumTags );
	if( pxVLANField != NULL )
	{
		return vlantagGET_VID_FROM_TCI( pxVLANField->usTCI );
	}

	return ~0;
}

BaseType_t xVLANCTagCheckClass( NetworkBufferDescriptor_t * pxBuf, BaseType_t xClass )
{
	BaseType_t xFoundClass = xVLANCTagGetPCP( pxBuf );
	return ( ( xFoundClass != ~0 ) && ( xFoundClass == xClass ) ) ? pdTRUE : pdFALSE;
}


BaseType_t xVLANCTagSetPCP( NetworkBufferDescriptor_t * pxBuf, BaseType_t xValue )
{
	struct xVLAN_TAG * pxVLANField = prvPrepareAndGetVLANCTag( pxBuf );
	if( pxVLANField != NULL )
	{
		vlantagSET_PCP_FROM_TCI( pxVLANField->usTCI, xValue );
		return pdTRUE;
	}
	return pdFALSE;
}

BaseType_t xVLANCTagSetDEI( NetworkBufferDescriptor_t * pxBuf, BaseType_t xValue )
{
	struct xVLAN_TAG * pxVLANField = prvPrepareAndGetVLANCTag( pxBuf );
	if( pxVLANField != NULL )
	{
		vlantagSET_DEI_FROM_TCI( pxVLANField->usTCI, xValue );
		return pdTRUE;
	}
	return pdFALSE;
}

BaseType_t xVLANCTagSetVID( NetworkBufferDescriptor_t * pxBuf, BaseType_t xValue )
{
	struct xVLAN_TAG * pxVLANField = prvPrepareAndGetVLANCTag( pxBuf );
	if( pxVLANField != NULL )
	{
		vlantagSET_VID_FROM_TCI( pxVLANField->usTCI, xValue );
		return pdTRUE;
	}
	return pdFALSE;
}


BaseType_t xVLANSTagSetPCP( NetworkBufferDescriptor_t * pxBuf, BaseType_t xValue )
{
	struct xVLAN_TAG * pxVLANField = prvPrepareAndGetVLANSTag( pxBuf );
	if( pxVLANField != NULL )
	{
		vlantagSET_PCP_FROM_TCI( pxVLANField->usTCI, xValue );
		return pdTRUE;
	}
	return pdFALSE;
}

BaseType_t xVLANSTagSetDEI( NetworkBufferDescriptor_t * pxBuf, BaseType_t xValue )
{
	struct xVLAN_TAG * pxVLANField = prvPrepareAndGetVLANSTag( pxBuf );
	if( pxVLANField != NULL )
	{
		vlantagSET_DEI_FROM_TCI( pxVLANField->usTCI, xValue );
		return pdTRUE;
	}
	return pdFALSE;
}

BaseType_t xVLANSTagSetVID( NetworkBufferDescriptor_t * pxBuf, BaseType_t xValue )
{
	struct xVLAN_TAG * pxVLANField = prvPrepareAndGetVLANSTag( pxBuf );
	if( pxVLANField != NULL )
	{
		vlantagSET_VID_FROM_TCI( pxVLANField->usTCI, xValue );
		return pdTRUE;
	}
	return pdFALSE;
}


