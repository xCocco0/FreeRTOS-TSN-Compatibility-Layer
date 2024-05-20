

/* Wrap around NetworkInterface.c but rename drivers functions and
 * hijack signals to IPTasks to our TSN Controller task
 */

#include "FreeRTOS_TSN_Controller.h"

#define xNetworkInterfaceInitialise xMAC_NetworkInterfaceInitialise
#define xNetworkInterfaceOutput     xMAC_NetworkInterfaceOutput
#define xGetPhyLinkStatus           xMAC_GetPhyLinkStatus
#define pxFillInterfaceDescriptor   pxMAC_FillInterfaceDescriptor

extern BaseType_t xSendEventStructToTSNController( const IPStackEvent_t * pxEvent,
                                                   TickType_t uxTimeout );
#define xSendEventStructToIPTask    xSendEventStructToTSNController

#include "NetworkInterface.c"

#undef pxFillInterfaceDescriptor
#undef xNetworkInterfaceInitialise
#undef xNetworkInterfaceOutput
#undef xGetPhyLinkStatus

#undef xSendEventStructToIPTask


/* Network interface wrapper function definitions
 */

BaseType_t xTSN_NetworkInterfaceInitialise( NetworkInterface_t * pxInterface )
{
	return xMAC_NetworkInterfaceInitialise( pxInterface );
}

BaseType_t xTSN_NetworkInterfaceOutput( NetworkInterface_t * pxInterface,
                                    NetworkBufferDescriptor_t * const pxBuffer,
                                    BaseType_t bReleaseAfterSend )
{
	return xMAC_NetworkInterfaceOutput( pxInterface, pxBuffer, bReleaseAfterSend );
}

#if defined( ipconfigIPv4_BACKWARD_COMPATIBLE ) && ( ipconfigIPv4_BACKWARD_COMPATIBLE == 1 )
    NetworkInterface_t * pxTSN_FillInterfaceDescriptor( BaseType_t xEMACIndex,
                                                    NetworkInterface_t * pxInterface )
	{
		return pxMAC_FillInterfaceDescriptor( xEMACIndex, pxInterface );
	}
#endif

BaseType_t xTSN_GetPhyLinkStatus( NetworkInterface_t * pxInterface )
{
	return xMAC_GetPhyLinkStatus( pxInterface );
}


/* Overwrite the default network interface with the previous
 */

BaseType_t xNetworkInterfaceInitialise( NetworkInterface_t * pxInterface )
{
    return xTSN_NetworkInterfaceInitialise( pxInterface );
}

BaseType_t xNetworkInterfaceOutput( NetworkInterface_t * pxInterface,
                                    NetworkBufferDescriptor_t * const pxBuffer,
                                    BaseType_t bReleaseAfterSend )
{
    return xTSN_NetworkInterfaceOutput( pxInterface, pxBuffer, bReleaseAfterSend );
}

#if defined( ipconfigIPv4_BACKWARD_COMPATIBLE ) && ( ipconfigIPv4_BACKWARD_COMPATIBLE == 1 )
    NetworkInterface_t * pxFillInterfaceDescriptor( BaseType_t xEMACIndex,
                                                    NetworkInterface_t * pxInterface )
    {
        return pxTSN_FillInterfaceDescriptor( xEMACIndex, pxInterface );
    }
#endif

BaseType_t xGetPhyLinkStatus( NetworkInterface_t * pxInterface )
{
    return xTSN_GetPhyLinkStatus( pxInterface );
}

