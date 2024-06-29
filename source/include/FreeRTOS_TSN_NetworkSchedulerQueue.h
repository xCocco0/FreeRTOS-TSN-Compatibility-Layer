#ifndef FREERTOS_TSN_NETWORK_SCHEDULER_QUEUE_H
#define FREERTOS_TSN_NETWORK_SCHEDULER_QUEUE_H

#include "FreeRTOS.h"
#include "queue.h"

#include "FreeRTOS_IP.h"

#include "FreeRTOS_TSN_Ancillary.h"

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
    eSendRecv,    /**< Queue send and receive events */
    eSendOnly,    /**< Only queue transmissions */
    eRecvOnly,    /**< Only queue receptions */
    eIPTaskEvents /**< Queue anything and forward to IP task queue */
} eQueuePolicy_t;

/** @brief The structure used in the network scheduler queues
 *
 * eEventType should be either eNetworkTxEvent for transmissions or
 * eNetworkRxEvent for receptions.
 * An additional remark for pxMsgh and pxBuf:
 * - For receptions, the ancillary message should always be present. This
 *   is because we are reusing Plus TCP structures, and the UDP packet list
 *   inside the TSN socket will only store one pointer. In the normal sockets
 *   this list contains pointers to network buffer descriptors, in TSN sockets
 *   it will store pointers to message headers.
 * - For transmissions, we don't currently support sendmsg() and therefore it
 *   is allowed to leave this field as NULL.
 * - In any case, if we are carrying an ancillary message, its iovec buffer
 *   should always point to the network buffer descriptor. When passing the
 *   queue item to the Plus TCP functions, pxBuf is rewritten to point to the
 *   ancillary msg, and then the original network buffer can be retrieved by
 *   accessing the iovec buffer of the msghdr.
 *   ```
 *   pxBuf->pucEthernetBuffer == pxMsgh
 *   pxMsgh->msg_iov[ 0 ].iov_base == pucOriginalEtherBuffer
 *   ```
 */
struct xNETQUEUE_ITEM
{
    eIPEvent_t eEventType; /**< Specifies whether this is packet is a transmittion or reception */
    NetworkBufferDescriptor_t * pxBuf; /** Pointer to the network buffer holding the data */
    struct msghdr * pxMsgh; /**< Pointer to message header holding ancillary data */
    BaseType_t xReleaseAfterSend; /**< Boolean specifying whether the network buffer should be released after its usage */
};

typedef struct xNETQUEUE_ITEM NetworkQueueItem_t;

/** @brief A network queue structure, a leaf in the network scheduler tree.
 *
 * This is wrapper to a basic FreeRTOS queue. In addition to that, it
 * contains:
 * - A queuing policy, that specifies whether this is queue is limited to
 *   store outbound and/or inbound packets. eIPTaskEvents is a special flag
 *   to hint the network scheduler to reuse the network event queue inside
 *   the Plus TCP addon. If the destination is TSN socket this will behave
 *   like eSendRecv. Note that unsupported traffic (i.e. ARP, TCP) will always
 *   be passed to Plus TCP.
 * - An internal priority value, this should in thery be the maximum priority of
 *   tasks which should receive/send packet on this queue. This is currently
 *   used when a packet matches multiple queue policy, so that the queue with
 *   the highest IPV is chosen. This field can be used to enable a dynamic
 *   priority for the TSN controller tasks, if the respective config entry is
 *   enabled: the controller assumes a priority which is equal to the maximum
 *   IPV among all the queues which have waiting packets.
 * - The filter function which restricts the type of packets that this queue
 *   is allowed to accept. This must be the signature of FilterFunction_t and
 *   return either pdTRUE or pdFALSE.
 * - The name field is currently unused in the socket API, but it can be used
 *   to insert a packet in a specific queue, without letting the scheduler
 *   decide on its own.
 */
struct xNETQUEUE
{
    QueueHandle_t xQueue;                          /**< FreeRTOS queue handle */
    UBaseType_t uxIPV;                             /**< Internal priority value */
    eQueuePolicy_t ePolicy;                        /**< Policy for message direction */
    #if ( tsnconfigMAX_QUEUE_NAME_LEN != 0 )
        char cName[ tsnconfigMAX_QUEUE_NAME_LEN ]; /**< Name of the queue */
    #endif
    FilterFunction_t fnFilter;                     /**< Function to filter incoming packets */
    #if ( tsnconfigINCLUDE_QUEUE_EVENT_CALLBACKS != tsnconfigDISABLE )
        PacketHandleFunction_t fnOnPop;            /**< Function to be called on packet pop */
        PacketHandleFunction_t fnOnPush;           /**< Function to be called on packet push */
    #endif
};

typedef struct xNETQUEUE NetworkQueue_t;

#if ( configSUPPORT_DYNAMIC_ALLOCATION != 0 )

    NetworkQueue_t * pxNetworkQueueMalloc();

    NetworkQueue_t * pxNetworkQueueCreate( eQueuePolicy_t ePolicy,
                                           UBaseType_t uxIPV,
                                           char * cName,
                                           FilterFunction_t fnFilter );

    void vNetworkQueueFree( NetworkQueue_t * pxQueue );

    NetworkQueueItem_t * pxNetworkQueueItemMalloc();

    void NetworkQueueItemFree( NetworkQueueItem_t * pxItem );

#endif /* if ( configSUPPORT_DYNAMIC_ALLOCATION != 0 ) */

UBaseType_t uxNetworkQueuePacketsWaiting( NetworkQueue_t * pxQueue );

BaseType_t xNetworkQueueIsEmpty( NetworkQueue_t * pxQueue );

#endif /* FREERTOS_TSN_NETWORK_SCHEDULER_QUEUE_H */
