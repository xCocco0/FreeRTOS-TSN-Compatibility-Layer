
#ifndef FREERTOS_TSN_NETWORK_SCHEDULER_QUEUE_H
#define FREERTOS_TSN_NETWORK_SCHEDULER_QUEUE_H

#include "FreeRTOS.h"
#include "queue.h"

#include "FreeRTOS_IP.h"

#include "FreeRTOSTSNConfig.h"
#include "FreeRTOSTSNConfigDefaults.h"


/* Function pointer to a filtering function.
 * Used to assign a packet to a network queue.
 * It should be defined by the user together with queue initialization.
 * Returns pdTRUE if the packet passes the filter, pdFALSE if not.
 */
typedef BaseType_t ( * FilterFunction_t ) ( NetworkBufferDescriptor_t * pxNetworkBuffer );

typedef BaseType_t ( * PacketHandleFunction_t ) ( NetworkBufferDescriptor_t * pxBuf );

typedef enum
{
    eSendRecv,      /**< Queue send and receive events */
    eSendOnly,      /**< Only queue transmissions */
    eRecvOnly,      /**< Only queue receptions */
	eIPTaskEvents   /**< Queue anything and forward to IP task queue */
} eQueuePolicy_t;

struct xNETQUEUE_ITEM
{
	eIPEvent_t eEventType;
	void * pvData;
	BaseType_t xReleaseAfterSend;
};

typedef struct xNETQUEUE_ITEM NetworkQueueItem_t;

struct xNETQUEUE {
	QueueHandle_t xQueue;                           /**< FreeRTOS queue handle */
	UBaseType_t uxIPV;                              /**< Internal priority value */
	eQueuePolicy_t ePolicy;                         /**< Policy for message direction */
	#if ( tsnconfigMAX_QUEUE_NAME_LEN != 0 )
		char cName[ tsnconfigMAX_QUEUE_NAME_LEN ];  /**< Name of the queue */
	#endif
	FilterFunction_t fnFilter;                      /**< Function to filter incoming packets */
	#if ( tsnconfigINCLUDE_QUEUE_EVENT_CALLBACKS != tsnconfigDISABLE )
		PacketHandleFunction_t fnOnPop;             /**< Function to be called on packet pop */
		PacketHandleFunction_t fnOnPush;            /**< Function to be called on packet push */
	#endif 
	TickType_t uxTimeout;                           /**< Timeout for push operations */
};

typedef struct xNETQUEUE NetworkQueue_t;

#if ( configSUPPORT_DYNAMIC_ALLOCATION != 0 )

NetworkQueue_t * pxNetworkQueueCreate();

void vNetworkQueueRelease( NetworkQueue_t * pxQueue );

#endif

#endif /* FREERTOS_TSN_NETWORK_SCHEDULER_QUEUE_H */
