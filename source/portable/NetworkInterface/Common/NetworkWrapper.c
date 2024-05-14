

#define pxFillInterfaceDescriptor pxMAC_FillInterfaceDescriptor
#include "NetworkInterface.c"
#undef pxFillInterfaceDescriptor


NetworkInterface_t * pxFillInterfaceDescriptor( BaseType_t xEMACIndex,
												NetworkInterface_t * pxInterface )
{
	return pxMAC_FillInterfaceDescriptor( xEMACIndex, pxInterface );
}


