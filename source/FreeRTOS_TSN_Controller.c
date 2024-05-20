
#include "FreeRTOS.h"

#include "FreeRTOS_IP.h"
#include "FreeRTOS_IP_Private.h"

#include "FreeRTOS_TSN_Controller.h"
#include "FreeRTOS_TSN_NetworkQueues.h"

BaseType_t xSendEventStructToTSNController( const IPStackEvent_t * pxEvent,
                                     TickType_t uxTimeout )
{
	BaseType_t xReturn = pdFALSE;

    if( pxEvent->eEventType == eNetworkRxEvent )
	{
		xReturn = xNetworkQueueInsertPacket( ( NetworkBufferDescriptor_t * ) pxEvent->pvData );
	}
	else
	{
		xReturn = xSendEventStructToIPTask( pxEvent, uxTimeout );
	}

	return xReturn;
}

static void prvTSNController_Initialise( void ) {}

static void prvTSNController( void * pvParameters ) {}
