/**
 * @file FreeRTOS_TSN_VLANTags.c
 *
 * @brief Implementation of functions for handling VLAN tags in FreeRTOS TSN Compatibility Layer.
 */

#include <string.h>

#include "FreeRTOS.h"

#include "FreeRTOS_IP.h"

#include "FreeRTOS_TSN_VLANTags.h"

/**
 * @brief Get the number of VLAN tags in the given network buffer.
 *
 * This function checks the Ethernet frame type in the network buffer and determines the number of VLAN tags present.
 *
 * @param pxBuf Pointer to the network buffer descriptor.
 *
 * @return The number of VLAN tags found in the network buffer.
 */
uint8_t ucGetNumberOfTags( NetworkBufferDescriptor_t * pxBuf )
{
	if( pxBuf->xDataLength < ipSIZE_OF_ETH_HEADER )
	{
		return 0;
	}

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

/**
 * @brief Get a pointer to the VLAN S-Tag in the network buffer.
 *
 * This function calculates the offset of the VLAN S-Tag in the network buffer based on the number of VLAN tags and returns a pointer to it.
 *
 * @param pxBuf Pointer to the network buffer descriptor.
 * @param ucNumTags The number of VLAN tags in the network buffer.
 *
 * @return Pointer to the VLAN S-Tag, or NULL if the VLAN S-Tag is not present.
 */
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

/**
 * @brief Get a pointer to the VLAN C-Tag in the network buffer.
 *
 * This function calculates the offset of the VLAN C-Tag in the network buffer based on the number of VLAN tags and returns a pointer to it.
 *
 * @param pxBuf Pointer to the network buffer descriptor.
 * @param ucNumTags The number of VLAN tags in the network buffer.
 *
 * @return Pointer to the VLAN C-Tag, or NULL if the VLAN C-Tag is not present.
 */
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

/**
 * @brief Prepare and get a pointer to the VLAN C-Tag in the network buffer.
 *
 * This function prepares the VLAN C-Tag in the network buffer if necessary and returns a pointer to it.
 *
 * @param pxBuf Pointer to the network buffer descriptor.
 *
 * @return Pointer to the VLAN C-Tag, or NULL if the VLAN C-Tag cannot be prepared.
 */
struct xVLAN_TAG * prvPrepareAndGetVLANCTag( NetworkBufferDescriptor_t * pxBuf )
{
	struct xVLAN_TAG * pxVLANField;
	uint8_t ucNumTags = ucGetNumberOfTags( pxBuf );
	
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

/**
 * @brief Prepare and get a pointer to the VLAN S-Tag in the network buffer.
 *
 * This function prepares the VLAN S-Tag in the network buffer if necessary and returns a pointer to it.
 *
 * @param pxBuf Pointer to the network buffer descriptor.
 *
 * @return Pointer to the VLAN S-Tag, or NULL if the VLAN S-Tag cannot be prepared.
 */
struct xVLAN_TAG * prvPrepareAndGetVLANSTag( NetworkBufferDescriptor_t * pxBuf )
{
	struct xVLAN_TAG * pxVLANFieldC, * pxVLANFieldS;
	uint8_t ucNumTags = ucGetNumberOfTags( pxBuf );
	
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

/**
 * @brief Get the Priority Code Point (PCP) from the VLAN S-Tag in the network buffer.
 *
 * This function retrieves the PCP value from the VLAN S-Tag in the network buffer.
 *
 * @param pxBuf Pointer to the network buffer descriptor.
 *
 * @return The PCP value, or ~0 if the VLAN S-Tag is not present.
 */
BaseType_t xVLANSTagGetPCP( NetworkBufferDescriptor_t * pxBuf )
{
	uint8_t ucNumTags = ucGetNumberOfTags( pxBuf );
	const struct xVLAN_TAG * pxVLANField = prvGetVLANSTag( pxBuf, ucNumTags );
	if( pxVLANField != NULL )
	{
		return vlantagGET_PCP_FROM_TCI( pxVLANField->usTCI );
	}

	return ~0;
}

/**
 * @brief Get the Drop Eligible Indicator (DEI) from the VLAN S-Tag in the network buffer.
 *
 * This function retrieves the DEI value from the VLAN S-Tag in the network buffer.
 *
 * @param pxBuf Pointer to the network buffer descriptor.
 *
 * @return The DEI value, or ~0 if the VLAN S-Tag is not present.
 */
BaseType_t xVLANSTagGetDEI( NetworkBufferDescriptor_t * pxBuf )
{
	uint8_t ucNumTags = ucGetNumberOfTags( pxBuf );
	const struct xVLAN_TAG * pxVLANField = prvGetVLANSTag( pxBuf, ucNumTags );
	if( pxVLANField != NULL )
	{
		return vlantagGET_DEI_FROM_TCI( pxVLANField->usTCI );
	}

	return ~0;
}

/**
 * @brief Get the VLAN Identifier (VID) from the VLAN S-Tag in the network buffer.
 *
 * This function retrieves the VID value from the VLAN S-Tag in the network buffer.
 *
 * @param pxBuf Pointer to the network buffer descriptor.
 *
 * @return The VID value, or ~0 if the VLAN S-Tag is not present.
 */
BaseType_t xVLANSTagGetVID( NetworkBufferDescriptor_t * pxBuf )
{
	uint8_t ucNumTags = ucGetNumberOfTags( pxBuf );
	const struct xVLAN_TAG * pxVLANField = prvGetVLANSTag( pxBuf, ucNumTags );
	if( pxVLANField != NULL )
	{
		return vlantagGET_VID_FROM_TCI( pxVLANField->usTCI );
	}

	return ~0;
}

/**
 * @brief Check if the VLAN S-Tag in the network buffer has a specific PCP value.
 *
 * This function checks if the PCP value of the VLAN S-Tag in the network buffer matches the specified value.
 *
 * @param pxBuf Pointer to the network buffer descriptor.
 * @param xClass The PCP value to check against.
 *
 * @return pdTRUE if the PCP value matches, pdFALSE otherwise.
 */
BaseType_t xVLANSTagCheckClass( NetworkBufferDescriptor_t * pxBuf, BaseType_t xClass )
{
	BaseType_t xFoundClass = xVLANSTagGetPCP( pxBuf );
	return ( ( xFoundClass != ~0 ) && ( xFoundClass == xClass ) ) ? pdTRUE : pdFALSE;
}

/**
 * @brief Get the Priority Code Point (PCP) from the VLAN C-Tag in the network buffer.
 *
 * This function retrieves the PCP value from the VLAN C-Tag in the network buffer.
 *
 * @param pxBuf Pointer to the network buffer descriptor.
 *
 * @return The PCP value, or ~0 if the VLAN C-Tag is not present.
 */
BaseType_t xVLANCTagGetPCP( NetworkBufferDescriptor_t * pxBuf )
{
	uint8_t ucNumTags = ucGetNumberOfTags( pxBuf );
	const struct xVLAN_TAG * pxVLANField = prvGetVLANCTag( pxBuf, ucNumTags );
	if( pxVLANField != NULL )
	{
		return vlantagGET_PCP_FROM_TCI( pxVLANField->usTCI );
	}

	return ~0;
}

/**
 * @brief Get the Drop Eligible Indicator (DEI) from the VLAN C-Tag in the network buffer.
 *
 * This function retrieves the DEI value from the VLAN C-Tag in the network buffer.
 *
 * @param pxBuf Pointer to the network buffer descriptor.
 *
 * @return The DEI value, or ~0 if the VLAN C-Tag is not present.
 */
BaseType_t xVLANCTagGetDEI( NetworkBufferDescriptor_t * pxBuf )
{
	uint8_t ucNumTags = ucGetNumberOfTags( pxBuf );
	const struct xVLAN_TAG * pxVLANField = prvGetVLANCTag( pxBuf, ucNumTags );
	if( pxVLANField != NULL )
	{
		return vlantagGET_DEI_FROM_TCI( pxVLANField->usTCI );
	}

	return ~0;
}

/**
 * @brief Get the VLAN Identifier (VID) from the VLAN C-Tag in the network buffer.
 *
 * This function retrieves the VID value from the VLAN C-Tag in the network buffer.
 *
 * @param pxBuf Pointer to the network buffer descriptor.
 *
 * @return The VID value, or ~0 if the VLAN C-Tag is not present.
 */
BaseType_t xVLANCTagGetVID( NetworkBufferDescriptor_t * pxBuf )
{
	uint8_t ucNumTags = ucGetNumberOfTags( pxBuf );
	const struct xVLAN_TAG * pxVLANField = prvGetVLANCTag( pxBuf, ucNumTags );
	if( pxVLANField != NULL )
	{
		return vlantagGET_VID_FROM_TCI( pxVLANField->usTCI );
	}

	return ~0;
}

/**
 * @brief Check if the VLAN C-Tag in the network buffer has a specific PCP value.
 *
 * This function checks if the PCP value of the VLAN C-Tag in the network buffer matches the specified value.
 *
 * @param pxBuf Pointer to the network buffer descriptor.
 * @param xClass The PCP value to check against.
 *
 * @return pdTRUE if the PCP value matches, pdFALSE otherwise.
 */
BaseType_t xVLANCTagCheckClass( NetworkBufferDescriptor_t * pxBuf, BaseType_t xClass )
{
	BaseType_t xFoundClass = xVLANCTagGetPCP( pxBuf );
	return ( ( xFoundClass != ~0 ) && ( xFoundClass == xClass ) ) ? pdTRUE : pdFALSE;
}


/**
 * @brief Set the Priority Code Point (PCP) of the VLAN C-Tag in the network buffer.
 *
 * This function sets the PCP value of the VLAN C-Tag in the network buffer.
 *
 * @param pxBuf Pointer to the network buffer descriptor.
 * @param xValue The PCP value to set.
 *
 * @return pdTRUE if the PCP value is set successfully, pdFALSE otherwise.
 */
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

/**
 * @brief Set the Drop Eligible Indicator (DEI) of the VLAN C-Tag in the network buffer.
 *
 * This function sets the DEI value of the VLAN C-Tag in the network buffer.
 *
 * @param pxBuf Pointer to the network buffer descriptor.
 * @param xValue The DEI value to set.
 *
 * @return pdTRUE if the DEI value is set successfully, pdFALSE otherwise.
 */
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

/**
 * @brief Set the VLAN Identifier (VID) of the VLAN C-Tag in the network buffer.
 *
 * This function sets the VID value of the VLAN C-Tag in the network buffer.
 *
 * @param pxBuf Pointer to the network buffer descriptor.
 * @param xValue The VID value to set.
 *
 * @return pdTRUE if the VID value is set successfully, pdFALSE otherwise.
 */
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


/**
 * @brief Set the Priority Code Point (PCP) of the VLAN S-Tag in the network buffer.
 *
 * This function sets the PCP value of the VLAN S-Tag in the network buffer.
 *
 * @param pxBuf Pointer to the network buffer descriptor.
 * @param xValue The PCP value to set.
 *
 * @return pdTRUE if the PCP value is set successfully, pdFALSE otherwise.
 */
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

/**
 * @brief Set the Drop Eligible Indicator (DEI) of the VLAN S-Tag in the network buffer.
 *
 * This function sets the DEI value of the VLAN S-Tag in the network buffer.
 *
 * @param pxBuf Pointer to the network buffer descriptor.
 * @param xValue The DEI value to set.
 *
 * @return pdTRUE if the DEI value is set successfully, pdFALSE otherwise.
 */
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

/**
 * @brief Set the VLAN Identifier (VID) of the VLAN S-Tag in the network buffer.
 *
 * This function sets the VID value of the VLAN S-Tag in the network buffer.
 *
 * @param pxBuf Pointer to the network buffer descriptor.
 * @param xValue The VID value to set.
 *
 * @return pdTRUE if the VID value is set successfully, pdFALSE otherwise.
 */
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


