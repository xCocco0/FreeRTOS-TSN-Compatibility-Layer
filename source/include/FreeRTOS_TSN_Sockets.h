
#ifndef FREERTOS_TSN_SOCKETS_H
#define FREERTOS_TSN_SOCKETS_H

#include "FreeRTOS.h"

#include "FreeRTOS_Sockets.h"

#define FREERTOS_TSN_INVALID_SOCKET ( ( TSNSocket_t ) ~0U )

#define FREERTOS_SO_VLAN_CTAG     ( 101 )
#define FREERTOS_SO_VLAN_TAG      FREERTOS_SO_VLAN_CTAG
#define FREERTOS_SO_VLAN_STAG     ( 102 )
#define FREERTOS_SO_VLAN_TAG_RST  ( 103 )

#define FREERTOS_SO_DS_CLASS   ( 104 )

/* Generic values for the DiffServ class */
#define FREERTOS_DF                  ( ( 0 ) << 4 )
/* FREERTOS_CSx is either 0,8,16,24,32,40,48,54 (CS0 = DF)*/
#define FREERTOS_CSx( x )            ( ( ( 0 <= x && x <= 7 ) ? ( 8 * x ) : FREERTOS_DF ) << 4 )
/* FREERTOS_AFxy_INET is either 10,12,14,18,20,22,26,28,30,34,36,38*/
#define FREERTOS_AFxy( x, y )        ( ( ( 1 <= x && x <= 4 && 1 <= y && y <= 3 ) ? ( 8 * x + 2 * y ) : FREERTOS_DF ) << 4 )
#define FREERTOS_LE                  ( ( 1 ) << 4 )
#define FREERTOS_EF                  ( ( 46 ) << 4 )
#define FREERTOS_DSCP_CUSTOM( x )    ( ( x & 0x3F ) << 4 ) /**< max 6 bits (0b11 1111)*/

#define FREERTOS_INET                ( 2 )
#define FREERTOS_INET6               ( 10 )

/* IPv4 compatible values */
#define FREERTOS_DF_INET             ( FREERTOS_DF | FREERTOS_INET )
#define FREERTOS_CSx_INET( x )       ( FREERTOS_CSx( x ) | FREERTOS_INET )
#define FREERTOS_AFxy_INET( x, y )   ( FREERTOS_AFxy( x, y ) | FREERTOS_INET )
#define FREERTOS_LE_INET             ( FREERTOS_LE | FREERTOS_INET )
#define FREERTOS_EF_INET             ( FREERTOS_EF | FREERTOS_INET )

/* IPv6 compatible values */
#define FREERTOS_DF_INET6            ( FREERTOS_DF | FREERTOS_INET6 )
#define FREERTOS_CSx_INET6( x )      ( FREERTOS_CSx( x ) | FREERTOS_INET6 )
#define FREERTOS_AFxy_INET6( x, y )  ( FREERTOS_AFxy( x, y ) | FREERTOS_INET6 )
#define FREERTOS_LE_INET6            ( FREERTOS_LE | FREERTOS_INET6 )
#define FREERTOS_EF_INET6            ( FREERTOS_EF | FREERTOS_INET6 )

struct xTSN_SOCKET
{
	Socket_t xBaseSocket;     /**< Reuse the same socket structure as Plus-TCP addon */

	uint8_t ucVLANTagsCount;  /**< The number of tags used, should be 0, 1 or 2 */
	uint8_t ucVLANCTagTCI;    /**< VLAN TCI field for the customer VLAN Tag */
	uint8_t ucVLANSTagTCI;    /**< VLAN TCI field for the service VLAN Tag of double tagged frames */

	uint8_t ucDSClass;        /**< Differentiated services class */

	TaskHandle_t xSendTask;   /**< Task handle of the task who is sending ( should always be at most one ) */
	TaskHandle_t xRecvTask;   /**< Task handle of the task who is receiving ( should always be at most one ) */
};

typedef struct xTSN_SOCKET * TSNSocket_t;

typedef struct xTSN_SOCKET FreeRTOS_TSN_Socket_t;

TSNSocket_t FreeRTOS_TSN_socket( BaseType_t xDomain,
						  BaseType_t xType,
						  BaseType_t xProtocol );

BaseType_t FreeRTOS_TSN_setsockopt( TSNSocket_t xSocket,
                                int32_t lLevel,
                                int32_t lOptionName,
                                const void * pvOptionValue,
                                size_t uxOptionLength );

BaseType_t FreeRTOS_TSN_bind( TSNSocket_t xSocket,
                          struct freertos_sockaddr const * pxAddress,
                          socklen_t xAddressLength );

int32_t FreeRTOS_TSN_sendto( TSNSocket_t xSocket,
                         const void * pvBuffer,
                         size_t uxTotalDataLength,
                         BaseType_t xFlags,
                         const struct freertos_sockaddr * pxDestinationAddress,
                         socklen_t xDestinationAddressLength );

int32_t FreeRTOS_TSN_recvfrom( TSNSocket_t xSocket,
                           void * pvBuffer,
                           size_t uxBufferLength,
                           BaseType_t xFlags,
                           struct freertos_sockaddr * pxSourceAddress,
                           socklen_t * pxSourceAddressLength );


#endif /* FREERTOS_TSN_SOCKETS_H */





