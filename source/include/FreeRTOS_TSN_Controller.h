#ifndef FREERTOS_TSN_CONTROLLER
#define FREERTOS_TSN_CONTROLLER

#include "FreeRTOS.h"
#include "FreeRTOS_IP.h"

BaseType_t xNotifyController();

void vTSNControllerComputePriority( void );

BaseType_t xTSNControllerUpdatePriority( UBaseType_t uxPriority );

void prvTSNController_Initialise( void );

BaseType_t xIsCallingFromTSNController( void );

#endif /* ifndef FREERTOS_TSN_CONTROLLER */
