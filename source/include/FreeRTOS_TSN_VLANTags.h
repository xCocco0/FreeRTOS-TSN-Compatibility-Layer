
#ifndef FREERTOS_TSN_VLAN_TAGS_H
#define FREERTOS_TSN_VLAN_TAGS_H

#include "FreeRTOS.h"

#include "FreeRTOS_IP.h"

#define vlantagCLASS_0 0U
#define vlantagCLASS_1 1U
#define vlantagCLASS_2 2U
#define vlantagCLASS_3 3U
#define vlantagCLASS_4 4U
#define vlantagCLASS_5 5U
#define vlantagCLASS_6 6U
#define vlantagCLASS_7 7U

#define vlantagETH_TAG_OFFSET ( 12U )

#define vlantagTPID_DEFAULT     ( 0x8100U )
#define vlantagTPID_DOUBLE_TAG  ( 0x88a8U )

#define vlantagPCP_BIT_MASK ( 0xE000U )
#define vlantagDEI_BIT_MASK ( 0x1000U )
#define vlantagVID_BIT_MASK ( 0x0FFFU )

#define vlantagGET_PCP_FROM_TCI( x ) ( ( x & vlantagPCP_BIT_MASK ) >> 13 )
#define vlantagGET_DEI_FROM_TCI( x ) ( ( x & vlantagDEI_BIT_MASK ) >> 12 )
#define vlantagGET_VID_FROM_TCI( x ) ( ( x & vlantagVID_BIT_MASK ) )

#define vlantagSET_PCP_FROM_TCI( x, value ) do \
	x = ( ( x & ~vlantagPCP_BIT_MASK ) | ( ( FreeRTOS_htons( value ) & 0x3U ) << 13 ) ); \
	while( 0 )
#define vlantagSET_DEI_FROM_TCI( x, value ) do \
	x = ( ( x & ~vlantagDEI_BIT_MASK ) | ( ( FreeRTOS_htons ( value ) & 0x1U ) >> 12 ) ); \
	while( 0 )
#define vlantagSET_VID_FROM_TCI( x, value ) do \
	x = ( ( x & ~vlantagVID_BIT_MASK ) | ( ( FreeRTOS_htons( value ) & 0xFFFU ) ) ); \
	while( 0 )

#include "pack_struct_start.h"
struct xVLAN_TAG
{
	uint16_t usTPID;
	uint16_t usTCI;
}
#include "pack_struct_end.h"

#include "pack_struct_start.h"
struct xTAGGED_ETH_HEADER
{
    MACAddress_t xDestinationAddress; /**< Destination address  0 + 6 = 6  */
    MACAddress_t xSourceAddress;      /**< Source address       6 + 6 = 12 */
	struct xVLAN_TAG xVLANTag;
    uint16_t usFrameType;             /**< The EtherType field 12 + 2 = 14 */
}
#include "pack_struct_end.h"

typedef struct xTAGGED_ETH_HEADER TaggedEthernetHeader_t;

#include "pack_struct_start.h"
struct xDOUBLE_TAGGED_ETH_HEADER
{
    MACAddress_t xDestinationAddress; /**< Destination address  0 + 6 = 6  */
    MACAddress_t xSourceAddress;      /**< Source address       6 + 6 = 12 */
	struct xVLAN_TAG xVLANSTag;
	struct xVLAN_TAG xVLANCTag;
    uint16_t usFrameType;             /**< The EtherType field 12 + 2 = 14 */
}
#include "pack_struct_end.h"

typedef struct xDOUBLE_TAGGED_ETH_HEADER DoubleTaggedEthernetHeader_t;


BaseType_t xVLANSTagGetPCP( NetworkBufferDescriptor_t * pxBuf );
BaseType_t xVLANSTagGetDEI( NetworkBufferDescriptor_t * pxBuf );
BaseType_t xVLANSTagGetVID( NetworkBufferDescriptor_t * pxBuf );
BaseType_t xVLANSTagCheckClass( NetworkBufferDescriptor_t * pxBuf, BaseType_t xClass );

BaseType_t xVLANSTagGetPCP( NetworkBufferDescriptor_t * pxBuf );
BaseType_t xVLANSTagGetDEI( NetworkBufferDescriptor_t * pxBuf );
BaseType_t xVLANSTagGetVID( NetworkBufferDescriptor_t * pxBuf );
BaseType_t xVLANSTagCheckClass( NetworkBufferDescriptor_t * pxBuf, BaseType_t xClass );

/* Defaults to customer tag
 */
#define xVLANTagGetPCP     xVLANCTagGetPCP
#define xVLANTagGetDEI     xVLANCTagGetDEI
#define xVLANTagGetVID     xVLANCTagGetVID
#define xVLANTagCheckClass xVLANCTagCheckClass


BaseType_t xVLANCTagSetPCP( NetworkBufferDescriptor_t * pxBuf, BaseType_t xValue );
BaseType_t xVLANCTagSetDEI( NetworkBufferDescriptor_t * pxBuf, BaseType_t xValue );
BaseType_t xVLANCTagSetVID( NetworkBufferDescriptor_t * pxBuf, BaseType_t xValue );

BaseType_t xVLANSTagSetPCP( NetworkBufferDescriptor_t * pxBuf, BaseType_t xValue );
BaseType_t xVLANSTagSetDEI( NetworkBufferDescriptor_t * pxBuf, BaseType_t xValue );
BaseType_t xVLANSTagSetVID( NetworkBufferDescriptor_t * pxBuf, BaseType_t xValue );

#define xVLANTagSetPCP xVLANCTagSetPCP
#define xVLANTagSetDEI xVLANCTagSetDEI
#define xVLANTagSetVID xVLANCTagSetVID

#endif /* FREERTOS_TSN_VLAN_TAGS_H */
