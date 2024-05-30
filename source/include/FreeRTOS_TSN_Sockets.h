
#ifndef FREERTOS_TSN_SOCKETS_H
#define FREERTOS_TSN_SOCKETS_H

#include "FreeRTOS.h"

#include "FreeRTOS_Sockets.h"

#define FREERTOS_TSN_INVALID_SOCKET ( ( TSNSocket_t ) ~0U )

#define FREERTOS_SO_VLAN_CTAG_PCP     ( 101 )
#define FREERTOS_SO_VLAN_TAG_PCP      FREERTOS_SO_VLAN_CTAG
#define FREERTOS_SO_VLAN_STAG_PCP     ( 102 )
#define FREERTOS_SO_VLAN_TAG_RST      ( 103 )

#define FREERTOS_SO_DS_CLASS   ( 104 )

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





