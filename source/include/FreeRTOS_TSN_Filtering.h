
#ifndef FREERTOS_TSN_FILTERING_H
#define FREERTOS_TSN_FILTERING_H

#include "FreeRTOS_IP.h"

/* Function pointer to a filtering function.
 * Used to assign a packet to a network queue.
 * It should be defined by the user together with queue initialization.
 * Returns pdTRUE if the packet passes the filter, pdFALSE if not.
 */
typedef BaseType_t ( *FilterFunction_t ) ( NetworkBufferDescriptor_t *pxNetworkBuffer );

#endif /* FREERTOS_TSN_FILTERING_H */
