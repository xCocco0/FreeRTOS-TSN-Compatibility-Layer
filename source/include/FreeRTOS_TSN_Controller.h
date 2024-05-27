
#ifndef FREERTOS_TSN_CONTROLLER
#define FREERTOS_TSN_CONTROLLER

#include "FreeRTOS.h"
#include "FreeRTOS_IP.h"

#define controllerMAX_EVENT_WAIT_TIME 1000 /**< max sleep time for controller in ms while waiting for events*/

BaseType_t xSendEventStructToTSNController( const IPStackEvent_t * pxEvent,
                                     TickType_t uxTimeout );

BaseType_t xNotifyController();

void vTSNControllerComputePriority( void );

BaseType_t xTSNControllerUpdatePriority( UBaseType_t uxPriority );

void prvTSNController_Initialise( void );

#endif
