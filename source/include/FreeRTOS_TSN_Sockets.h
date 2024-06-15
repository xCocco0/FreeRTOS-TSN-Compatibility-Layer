
#ifndef FREERTOS_TSN_SOCKETS_H
#define FREERTOS_TSN_SOCKETS_H

#include "FreeRTOS.h"

#include "list.h"

#include "FreeRTOS_Sockets.h"
#include "FreeRTOS_TSN_Ancillary.h"

#include "FreeRTOSTSNConfig.h"
#include "FreeRTOSTSNConfigDefaults.h"

#define FREERTOS_TSN_INVALID_SOCKET ( ( TSNSocket_t ) ~0U )

#if ( tsnconfigSOCKET_INSERTS_VLAN_TAGS != tsnconfigDISABLE ) //TODO: see SO_PRIORITY
    #define FREERTOS_SO_VLAN_CTAG_PCP    ( 101 )
    #define FREERTOS_SO_VLAN_TAG_PCP     FREERTOS_SO_VLAN_CTAG
    #define FREERTOS_SO_VLAN_STAG_PCP    ( 102 )
    #define FREERTOS_SO_VLAN_TAG_RST     ( 103 )
#endif

#define FREERTOS_SO_DS_CLASS             ( 104 )

#define FREERTOS_SO_TIMESTAMP_OLD        ( 29 )
#define FREERTOS_SO_TIMESTAMPNS_OLD      ( 35 )
#define FREERTOS_SO_TIMESTAMPING_OLD     ( 37 )

#define FREERTOS_SO_TIMESTAMP            FREERTOS_SO_TIMESTAMP_OLD
#define FREERTOS_SO_TIMESTAMPNS	         FREERTOS_SO_TIMESTAMPNS_OLD
#define FREERTOS_SO_TIMESTAMPING         FREERTOS_SO_TIMESTAMPING_OLD

#define FREERTOS_SCM_TIMESTAMP           FREERTOS_SO_TIMESTAMP
#define FREERTOS_SCM_TIMESTAMPNS         FREERTOS_SO_TIMESTAMPNS
#define FREERTOS_SCM_TIMESTAMPING        FREERTOS_SO_TIMESTAMPING

#define FREERTOS_SOL_SOCKET              ( 1 )
#define FREERTOS_SOL_IP                  ( 0 )
#define FREERTOS_SOL_IPV6                ( 41 )
#define FREERTOS_IP_RECVERR              ( 11 )
#define FREERTOS_IPV6_RECVERR            ( 25 )

#define FREERTOS_MSG_ERRQUEUE            ( 2 << 13 )

/* SO_TIMESTAMPING flags */
enum {
	SOF_TIMESTAMPING_TX_HARDWARE = (1<<0),
	SOF_TIMESTAMPING_TX_SOFTWARE = (1<<1),
	SOF_TIMESTAMPING_RX_HARDWARE = (1<<2),
	SOF_TIMESTAMPING_RX_SOFTWARE = (1<<3),
	SOF_TIMESTAMPING_SOFTWARE = (1<<4),
	SOF_TIMESTAMPING_SYS_HARDWARE = (1<<5),
	SOF_TIMESTAMPING_RAW_HARDWARE = (1<<6),
	SOF_TIMESTAMPING_OPT_ID = (1<<7),
	SOF_TIMESTAMPING_TX_SCHED = (1<<8),
	SOF_TIMESTAMPING_TX_ACK = (1<<9),
	SOF_TIMESTAMPING_OPT_CMSG = (1<<10),
	SOF_TIMESTAMPING_OPT_TSONLY = (1<<11),
	SOF_TIMESTAMPING_OPT_STATS = (1<<12),
	SOF_TIMESTAMPING_OPT_PKTINFO = (1<<13),
	SOF_TIMESTAMPING_OPT_TX_SWHW = (1<<14),
	SOF_TIMESTAMPING_BIND_PHC = (1 << 15),
	SOF_TIMESTAMPING_OPT_ID_TCP = (1 << 16),

	SOF_TIMESTAMPING_LAST = SOF_TIMESTAMPING_OPT_ID_TCP,
	SOF_TIMESTAMPING_MASK = (SOF_TIMESTAMPING_LAST - 1) |
				 SOF_TIMESTAMPING_LAST
};

/* The type of scm_timestamping, passed in sock_extended_err ee_info.
 * This defines the type of ts[0]. For SCM_TSTAMP_SND only, if ts[0]
 * is zero, then this is a hardware timestamp and recorded in ts[2].
 */
enum {
	SCM_TSTAMP_SND,		/* driver passed skb to NIC, or HW */
	SCM_TSTAMP_SCHED,	/* data entered the packet scheduler */
	SCM_TSTAMP_ACK,		/* data acknowledged by peer */
};

struct xTSN_SOCKET
{
	Socket_t xBaseSocket;     /**< Reuse the same socket structure as Plus-TCP addon */

	QueueHandle_t xErrQueue; /**< Contain errqueue with ancillary msgs*/

    #if ( tsnconfigSOCKET_INSERTS_VLAN_TAGS != tsnconfigDISABLE )
        uint8_t ucVLANTagsCount;  /**< The number of tags used, should be 0, 1 or 2 */
        uint16_t usVLANCTagTCI;    /**< VLAN TCI field for the customer VLAN Tag */
        uint16_t usVLANSTagTCI;    /**< VLAN TCI field for the service VLAN Tag of double tagged frames */
    #endif
	uint8_t ucDSClass;        /**< Differentiated services class */

    uint32_t ulTSFlags;       /**< Holds the timestamping config bits */

    ListItem_t xBoundSocketListItem; /** To keep track of TSN sockets */
	TaskHandle_t xSendTask;   /**< Task handle of the task who is sending ( should always be at most one ) */
	TaskHandle_t xRecvTask;   /**< Task handle of the task who is receiving ( should always be at most one ) */
};

typedef struct xTSN_SOCKET * TSNSocket_t;

typedef struct xTSN_SOCKET FreeRTOS_TSN_Socket_t;

void vInitialiseTSNSockets();

void vSocketFromPort( TickType_t xSearchKey, Socket_t * pxBaseSocket, TSNSocket_t * pxTSNSocket );

BaseType_t xSocketErrorQueueInsert( TSNSocket_t xTSNSocket, struct msghdr * pxMsgh );

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

BaseType_t FreeRTOS_TSN_closesocket( TSNSocket_t xSocket );

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

int32_t FreeRTOS_TSN_recvmsg( TSNSocket_t xSocket, struct msghdr * pxMsghUser, BaseType_t xFlags );

#endif /* FREERTOS_TSN_SOCKETS_H */





