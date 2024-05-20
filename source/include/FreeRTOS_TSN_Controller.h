
#ifndef FREERTOS_TSN_CONTROLLER
#define FREERTOS_TSN_CONTROLLER

#include "FreeRTOS.h"
#include "FreeRTOS_IP.h"

BaseType_t xSendEventStructToTSNController( const IPStackEvent_t * pxEvent,
                                     TickType_t uxTimeout );

BaseType_t xSendPacket( NetworkBufferDescriptor_t * pxBuf );

void prvTSNController_Initialise( void );

#endif
