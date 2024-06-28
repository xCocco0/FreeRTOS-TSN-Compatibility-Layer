/**
 * @file FreeRTOS_TSN_Ancillary.c
 * @brief Implementation of ancillary message functions for FreeRTOS+TCP.
 *
 * This file contains the implementation of ancillary message functions for FreeRTOS+TCP.
 * These functions are used to allocate, fill, and free ancillary messages, which are used
 * to pass control and data information between sockets.
 */
#include "FreeRTOS_TSN_Ancillary.h"

#include "FreeRTOS_IP.h"

#include "FreeRTOS_TSN_Sockets.h"
#include "FreeRTOS_TSN_Timestamp.h"


/// @brief Aligns the size of a control message buffer. 
portINLINE struct cmsghdr * __CMSG_NXTHDR( void * ctl,
                                           size_t size,
                                           struct cmsghdr * cmsg )
{
    struct cmsghdr * ptr;

    ptr = ( struct cmsghdr * ) ( ( ( unsigned char * ) cmsg ) + CMSG_ALIGN( cmsg->cmsg_len ) );

    if( ( unsigned long ) ( ( char * ) ( ptr + 1 ) - ( char * ) ctl ) > size )
    {
        return ( struct cmsghdr * ) 0;
    }

    return ptr;
}


/**
 * @brief Allocates memory for a new msghdr structure.
 *
 * This function allocates memory for a new msghdr structure using the pvPortMalloc function.
 * The allocated memory is then initialized with zeros using the memset function.
 *
 * @return A pointer to the newly allocated msghdr structure.
 */
struct msghdr * pxAncillaryMsgMalloc()
{
    struct msghdr * pxMsgh = pvPortMalloc( sizeof( struct msghdr ) );

    memset( pxMsgh, '\0', sizeof( struct msghdr ) );

    return pxMsgh;
}

/**
 * @brief Frees the memory allocated for an ancillary message.
 *
 * This function frees the memory allocated for the given ancillary message.
 *
 * @param pxMsgh Pointer to the `msghdr` structure representing the ancillary message.
 */
void vAncillaryMsgFree( struct msghdr * pxMsgh )
{
    vPortFree( pxMsgh );
}

/**
 * @brief Frees a msghdr
 *
 * This will free all the non null members of the msghdr. In order to make sense
 * it should always be used on a msghdr created using pxAncillaryMsgMalloc(),
 * which takes the duty of initializing the struct to zero. Also note that this
 * frees the iovec array, but not the iov_base buffers.
 *
 * @param pxMsgh Pointer to msghdr to free
 */
void vAncillaryMsgFreeAll( struct msghdr * pxMsgh )
{
    if( pxMsgh->msg_iov != NULL )
    {
        vPortFree( pxMsgh->msg_iov );
    }

    if( pxMsgh->msg_name != NULL )
    {
        vPortFree( pxMsgh->msg_name );
    }

    if( pxMsgh->msg_control != NULL )
    {
        vPortFree( pxMsgh->msg_control );
    }

    vAncillaryMsgFree( pxMsgh );
}

/**
 * @brief Fills in the name field of a message header structure with the given IP address, port, and family.
 *
 * This function is used to fill in the name field of a message header structure with the given IP address, port, and family.
 * The name field is used to specify the source or destination address of a socket.
 *
 * @param pxMsgh A pointer to the message header structure.
 * @param xAddr A pointer to the IP address to be filled in the name field.
 * @param usPort The port number to be filled in the name field.
 * @param xFamily The address family to be filled in the name field.
 *
 * @return pdPASS if the name field is successfully filled, pdFAIL otherwise.
 */
BaseType_t xAncillaryMsgFillName( struct msghdr * pxMsgh,
                                  IP_Address_t * xAddr,
                                  uint16_t usPort,
                                  BaseType_t xFamily )
{
    struct freertos_sockaddr * pxSockAddr;

    if( xAddr != NULL )
    {
        pxSockAddr = ( struct freertos_sockaddr * ) pvPortMalloc( sizeof( struct freertos_sockaddr ) );

        if( pxSockAddr == NULL )
        {
            return pdFAIL;
        }

        pxSockAddr->sin_len = sizeof( pxSockAddr );
        pxSockAddr->sin_address = *xAddr;
        pxSockAddr->sin_family = xFamily;
        /* port is in network bytes order for both input and output */
        pxSockAddr->sin_port = usPort;
        pxSockAddr->sin_flowinfo = 0;

        pxMsgh->msg_name = pxSockAddr;
        pxMsgh->msg_namelen = sizeof( pxSockAddr );
    }
    else
    {
        pxMsgh->msg_name = NULL;
        pxMsgh->msg_namelen = 0;
    }

    return pdPASS;
}

/**
 * @brief Frees the memory allocated for the name field in the given msghdr structure.
 *
 * This function frees the memory allocated for the name field in the provided msghdr structure.
 *
 * @param pxMsgh Pointer to the msghdr structure.
 */
void vAncillaryMsgFreeName( struct msghdr * pxMsgh )
{
    vPortFree( pxMsgh->msg_name );
}


/**
 * @brief Fills the payload of an ancillary message.
 *
 * This function fills the payload of an ancillary message with the provided buffer and length.
 *
 * @param pxMsgh Pointer to the msghdr structure representing the ancillary message.
 * @param pucBuffer Pointer to the buffer containing the payload data.
 * @param uxLength Length of the payload data in bytes.
 *
 * @return pdPASS if the payload was successfully filled, pdFAIL otherwise.
 */
BaseType_t xAncillaryMsgFillPayload( struct msghdr * pxMsgh,
                                     uint8_t * pucBuffer,
                                     size_t uxLength )
{
    struct iovec * pxIOvec;

    pxIOvec = pvPortMalloc( sizeof( struct iovec ) );

    if( pxIOvec == NULL )
    {
        return pdFAIL;
    }

    pxIOvec->iov_len = uxLength;
    pxIOvec->iov_base = pucBuffer;

    pxMsgh->msg_iov = pxIOvec;
    pxMsgh->msg_iovlen = 1;

    return pdPASS;
}

/**
 * @brief Frees the payload of an ancillary message.
 *
 * This function frees the memory allocated for the payload of an ancillary message.
 * The caller must ensure that the array is not empty before calling this function.
 *
 * @param pxMsgh Pointer to the msghdr structure representing the ancillary message.
 */
void vAncillaryMsgFreePayload( struct msghdr * pxMsgh )
{
    /* Note: the caller must check if the array is empty */

    for( BaseType_t uxIter = 0; uxIter < pxMsgh->msg_iovlen; ++uxIter )
    {
        vPortFree( pxMsgh->msg_iov[ uxIter ].iov_base );
    }

    vPortFree( pxMsgh->msg_iov );
}

/**
 * @brief Fills the ancillary message control structure with data.
 *
 * This function fills the ancillary message control structure with data provided in the input parameters.
 *
 * @param pxMsgh A pointer to the msghdr structure representing the message header.
 * @param pxCmsgVec A pointer to the cmsghdr structure representing the control message vector.
 * @param ppvDataVec An array of void pointers representing the data vector.
 * @param puxDataLenVec An array of size_t values representing the data length vector.
 * @param uxNumBuffers The number of buffers in the data vector.
 *
 * @return pdTRUE if the ancillary message control structure is successfully filled, pdFAIL otherwise.
 */
BaseType_t xAncillaryMsgControlFill( struct msghdr * pxMsgh,
                                     struct cmsghdr * pxCmsgVec,
                                     void ** ppvDataVec,
                                     size_t * puxDataLenVec,
                                     size_t uxNumBuffers )
{
    struct cmsghdr * pxCmsghIter;
    size_t uxTotalSpace = 0;

    if( pxMsgh == NULL )
    {
        return pdFALSE;
    }

    // Calculate the total space required for the ancillary message control structure
    for( size_t uxIter = 0; uxIter < uxNumBuffers; ++uxIter )
    {
        uxTotalSpace += CMSG_SPACE( puxDataLenVec[ uxIter ] );
    }

    /* Note: the address returned by portMalloc is already aligned by 8 bytes */
    uint8_t * pxBuffer = pvPortMalloc( uxTotalSpace );

    if( pxBuffer == NULL )
    {
        // Allocation failed, set the message control fields to NULL and return pdFAIL
        pxMsgh->msg_control = NULL;
        pxMsgh->msg_controllen = 0;
        return pdFAIL;
    }

    // Set the message control fields to the allocated buffer and the total space
    pxMsgh->msg_control = pxBuffer;
    pxMsgh->msg_controllen = uxTotalSpace;

    pxCmsghIter = CMSG_FIRSTHDR( pxMsgh );

    // Iterate through the data vector and fill the ancillary message control structure
    for( size_t uxIter = 0; uxIter < uxNumBuffers; ++uxIter )
    {
        if( pxCmsghIter == NULL )
        {
            // Free the allocated buffer and set the message control fields to NULL if there is no more space for control messages
            vPortFree( pxBuffer );
            pxMsgh->msg_control = NULL;
            pxMsgh->msg_controllen = 0;            
            return pdFAIL;
        }

        pxCmsghIter->cmsg_len = CMSG_LEN( puxDataLenVec[ uxIter ] );
        pxCmsghIter->cmsg_level = pxCmsgVec[ uxIter ].cmsg_level;
        pxCmsghIter->cmsg_type = pxCmsgVec[ uxIter ].cmsg_type;
        memcpy( CMSG_DATA( pxCmsghIter ), ppvDataVec[ uxIter ], puxDataLenVec[ uxIter ] );

        pxCmsghIter = CMSG_NXTHDR( pxMsgh, pxCmsghIter );
    }

    return pdTRUE;
}

/**
 * @brief Fills a single ancillary message control structure with data.
 *
 * This function fills a single ancillary message control structure with data.
 *
 * @param pxMsgh Pointer to the msghdr structure.
 * @param pxCmsg Pointer to the cmsghdr structure.
 * @param pvData Pointer to the data.
 * @param puxDataLen Length of the data.
 *
 * @return The result of the operation.
 */
BaseType_t xAncillaryMsgControlFillSingle( struct msghdr * pxMsgh,
                                           struct cmsghdr * pxCmsg,
                                           void * pvData,
                                           size_t puxDataLen );

/**
 * @brief Frees the memory allocated for the ancillary message control data.
 *
 * This function frees the memory allocated for the ancillary message control data
 * pointed to by the `msg_control` member of the `msghdr` structure.
 *
 * @param pxMsgh Pointer to the `msghdr` structure.
 */
void vAncillaryMsgFreeControl( struct msghdr * pxMsgh )
{
    vPortFree( pxMsgh->msg_control );
}
