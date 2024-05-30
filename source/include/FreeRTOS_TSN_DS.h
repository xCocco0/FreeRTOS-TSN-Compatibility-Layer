
#ifndef FREERTOS_TSN_DS_H
#define FREERTOS_TSN_DS_H

#include "FreeRTOS.h"

#include "FreeRTOS_IP.h"

#define diffservCLASS_DF                  ( 0 )
/* diffservCLASS_CSx is either 0,8,16,24,32,40,48,54 (CS0 = DF)*/
#define diffservCLASS_CSx( x )            ( ( 0 <= x && x <= 7 ) ? ( 8 * x ) : diffservCLASS_DF )
/* diffservCLASS_AFxy_INET is either 10,12,14,18,20,22,26,28,30,34,36,38*/
#define diffservCLASS_AFxy( x, y )        ( ( 1 <= x && x <= 4 && 1 <= y && y <= 3 ) ? ( 8 * x + 2 * y ) : diffservCLASS_DF )
#define diffservCLASS_LE                  ( 1 )
#define diffservCLASS_EF                  ( 46 )
#define diffservCLASS_DSCP_CUSTOM( x )    ( x & 0x3F ) /**< max 6 bits (0b11 1111)*/

#define diffservGET_DSCLASS_IPv4( pxIPHeader ) \
	( pxIPHeader->ucDifferentiatedServicesCode >> 2 )

#define diffservGET_DSCLASS_IPv6( pxIPHeader ) \
	( ( ( pxIPHeader->ucVersionTrafficClass & 0xF ) << 2 ) | ( ( pxIPHeader->ucTrafficClassFlow & 0xC0 ) >> 6 ) )

#define diffservSET_DSCLASS_IPv4( pxIPHeader, ucValue ) do { \
	pxIPHeader->ucDifferentiatedServicesCode = ( ucValue << 2 ); \
	} while( ipFALSE_BOOL )

#define diffservSET_DSCLASS_IPv6( pxIPHeader, ucValue ) do { \
	pxIPHeader->ucVersionTrafficClass &= 0xF0; \
	pxIPHeader->ucVersionTrafficClass |= ( ( ucValue & 0x3C ) >> 2 ); \
	pxIPHeader->ucTrafficClassFlow &= 0x3F; \
	pxIPHeader->ucTrafficClassFlow |= ( ( ucValue & 0x3 ) << 6 ); \
	} while( ipFALSE_BOOL )

	
uint8_t ucDSClassGet( NetworkBufferDescriptor_t * pxBuf );

BaseType_t xDSClassSet( NetworkBufferDescriptor_t * pxBuf, uint8_t ucValue );

#endif /* FREERTOS_TSN_DS_H */

