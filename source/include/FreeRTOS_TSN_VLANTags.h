
#ifndef FREERTOS_TSN_VLAN_TAGS_H
#define FREERTOS_TSN_VLAN_TAGS_H

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


BaseType_t xVLANTagGetPCP( NetworkBufferDescriptor_t * pxBuf );

BaseType_t xVLANTagGetDEI( NetworkBufferDescriptor_t * pxBuf );

BaseType_t xVLANTagGetVID( NetworkBufferDescriptor_t * pxBuf );

BaseType_t xVLANTagCheckTPID( NetworkBufferDescriptor_t * pxBuf );

BaseType_t xVLANTagCheckClass( NetworkBufferDescriptor_t * pxBuf, BaseType_t xClass );

#endif /* FREERTOS_TSN_VLAN_TAGS_H */
