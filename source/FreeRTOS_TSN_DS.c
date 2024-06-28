/**
 * @file FreeRTOS_TSN_DS.c
 * @brief FreeRTOS TSN Compatibility Layer - Data Structures
 *
 * This file contains the implementation of functions related to retrieving and setting DiffServ class in network buffers.
 */

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
void prvGetIPVersionAndOffset( NetworkBufferDescriptor_t * pxBuf,
                               uint16_t * pusIPVersion,
                               size_t * pulOffset )
{
    // Get a pointer to the Ethernet header
    EthernetHeader_t * pxEthHead = ( EthernetHeader_t * ) pxBuf->pucEthernetBuffer;
    
    // Get the Ethernet frame type
    uint16_t usEthType = FreeRTOS_ntohs( pxEthHead->usFrameType );

    // Set the initial offset to the size of the Ethernet header
    *pulOffset = ipSIZE_OF_ETH_HEADER;

    // Check if the Ethernet frame type is double VLAN tagged
    if( usEthType == vlantagTPID_DOUBLE_TAG )
    {
        // Increase the offset by the size of two VLAN tags
        *pulOffset += 2 * sizeof( struct xVLAN_TAG );
        
        // Get a pointer to the double tagged Ethernet header
        DoubleTaggedEthernetHeader_t * pxDTEthHead = ( DoubleTaggedEthernetHeader_t * ) pxBuf->pucEthernetBuffer;
        
        // Store the IP version
        *pusIPVersion = pxDTEthHead->usFrameType;
    }
    // Check if the Ethernet frame type is single VLAN tagged
    else if( usEthType == vlantagTPID_DEFAULT )
    {
        // Increase the offset by the size of a single VLAN tag
        *pulOffset += sizeof( struct xVLAN_TAG );
        
        // Get a pointer to the tagged Ethernet header
        TaggedEthernetHeader_t * pxTEthHead = ( TaggedEthernetHeader_t * ) pxBuf->pucEthernetBuffer;
        
        // Store the IP version
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

    // Get the IP version and offset from the network buffer
    prvGetIPVersionAndOffset( pxBuf, &usEtherType, &ulIPHeaderOffset );

    switch( usEtherType )
    {
        case ipIPv4_FRAME_TYPE:
            // If the EtherType is IPv4, cast the IP header and call diffservGET_DSCLASS_IPv4
            IPHeader_t * pxIPHeader = ( IPHeader_t * ) &pxBuf->pucEthernetBuffer[ ulIPHeaderOffset ];
            return diffservGET_DSCLASS_IPv4( pxIPHeader );

        case ipIPv6_FRAME_TYPE:
            // If the EtherType is IPv6, cast the IPv6 header and call diffservGET_DSCLASS_IPv6
            IPHeader_IPv6_t * pxIPv6Header = ( IPHeader_IPv6_t * ) &pxBuf->pucEthernetBuffer[ ulIPHeaderOffset ];
            return diffservGET_DSCLASS_IPv6( pxIPv6Header );

        default:
            // If the EtherType is neither IPv4 nor IPv6, return an invalid value
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
BaseType_t xDSClassSet( NetworkBufferDescriptor_t * pxBuf,
                        uint8_t ucValue )
{
    // Get the IP version and offset in the network buffer
    size_t ulIPHeaderOffset;
    uint16_t usEtherType;
    prvGetIPVersionAndOffset( pxBuf, &usEtherType, &ulIPHeaderOffset );

    switch( usEtherType )
    {
        case ipIPv4_FRAME_TYPE:
            // Set DiffServ class for IPv4
            IPHeader_t * pxIPHeader = ( IPHeader_t * ) &pxBuf->pucEthernetBuffer[ ulIPHeaderOffset ];
            diffservSET_DSCLASS_IPv4( pxIPHeader, ucValue );
            return pdPASS;

        case ipIPv6_FRAME_TYPE:
            // Set DiffServ class for IPv6
            IPHeader_IPv6_t * pxIPv6Header = ( IPHeader_IPv6_t * ) &pxBuf->pucEthernetBuffer[ ulIPHeaderOffset ];
            diffservSET_DSCLASS_IPv6( pxIPv6Header, ucValue );
            return pdPASS;

        default:
            // Invalid EtherType
            return pdFAIL;
    }
}
