
#ifndef FREERTOS_TSN_PTP_H
#define FREERTOS_TSN_PTP_H

#include "FreeRTOS.h"
#include "FreeRTOS_Sockets.h"

#include "pack_struct_start.h"
struct xPTP_HEADER
{
	uint8_t ucMajorSdoIDMessageType;
	uint8_t ucMinorVersionPTPVersionPTP;
	uint16_t usMessageLength;
	uint8_t ucDomainNumber;
	uint8_t ucMinorSdoID;
	uint16_t usFlags;
	uint8_t pucCorrectionField[8];
	uint32_t ulMessageTypeSpecific;
	uint8_t pucSourcePortIdentity[10];
	uint16_t usSequenceID;
	uint8_t ucControlField;
	uint8_t ucLogMessageInterval;
}
#include "pack_struct_end.h"

typedef struct xPTPHEADER PTPHeader_t;


void vSendPTPMessage( const struct freertos_sockaddr * pxDestinationAddress );

#endif
