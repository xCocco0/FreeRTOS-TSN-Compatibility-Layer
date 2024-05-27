
#ifndef NETWORK_WRAPPER_H
#define NETWORK_WRAPPER_H

#include "FreeRTOS.h"

#include "FreeRTOS_IP.h"


/* Definitions used to create the API exported to the upper layers
 */
BaseType_t xTSN_NetworkInterfaceInitialise( NetworkInterface_t * pxInterface );

BaseType_t xTSN_NetworkInterfaceOutput( NetworkInterface_t * pxInterface,
                                    NetworkBufferDescriptor_t * const pxBuffer,
                                    BaseType_t bReleaseAfterSend );;;;

#if defined( ipconfigIPv4_BACKWARD_COMPATIBLE ) && ( ipconfigIPv4_BACKWARD_COMPATIBLE == 1 )
    NetworkInterface_t * pxTSN_FillInterfaceDescriptor( BaseType_t xEMACIndex,
                                                    NetworkInterface_t * pxInterface );
#endif

BaseType_t xTSN_GetPhyLinkStatus( NetworkInterface_t * pxInterface );


/* Definitions used at this level to interface
 * the lower MAC layer
 */
BaseType_t xMAC_NetworkInterfaceInitialise( NetworkInterface_t * pxInterface );

BaseType_t xMAC_NetworkInterfaceOutput( NetworkInterface_t * pxInterface,
                                    NetworkBufferDescriptor_t * const pxBuffer,
                                    BaseType_t bReleaseAfterSend );;;;

#if defined( ipconfigIPv4_BACKWARD_COMPATIBLE ) && ( ipconfigIPv4_BACKWARD_COMPATIBLE == 1 )
    NetworkInterface_t * pxMAC_FillInterfaceDescriptor( BaseType_t xEMACIndex,
                                                    NetworkInterface_t * pxInterface );
#endif

BaseType_t xMAC_GetPhyLinkStatus( NetworkInterface_t * pxInterface );

#endif /* NETWORK_WRAPPER_H */
