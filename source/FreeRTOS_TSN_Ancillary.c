
#include "FreeRTOS_TSN_Ancillary.h"

#include "FreeRTOS_IP.h"

#include "FreeRTOS_TSN_Sockets.h"
#include "FreeRTOS_TSN_Timestamp.h"


portINLINE struct cmsghdr * __CMSG_NXTHDR( void *ctl, size_t size,
					       struct cmsghdr *cmsg )
{
	struct cmsghdr * ptr;
	ptr = ( struct cmsghdr * ) ( ( ( unsigned char * ) cmsg ) + CMSG_ALIGN( cmsg->cmsg_len ) );
	if ( ( unsigned long ) ( ( char * ) ( ptr + 1 ) - ( char * ) ctl) > size )
		return ( struct cmsghdr * ) 0;
	return ptr;
}


struct msghdr * pxAncillaryMsgMalloc()
{
    struct msghdr * pxMsgh = pvPortMalloc( sizeof( struct msghdr ) );
    memset( pxMsgh, '\0', sizeof( struct msghdr ) );
    
    return pxMsgh;
}

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

BaseType_t xAncillaryMsgFillName( struct msghdr * pxMsgh, IP_Address_t * xAddr, uint16_t usPort, BaseType_t xFamily )
{
    struct freertos_sockaddr * pxSockAddr;

    if( xAddr != NULL )
    {
        pxSockAddr = ( struct freertos_sockaddr * ) pvPortMalloc( sizeof ( struct freertos_sockaddr ) );
        
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

void vAncillaryMsgFreeName( struct msghdr * pxMsgh )
{
    vPortFree( pxMsgh->msg_name );
}


BaseType_t xAncillaryMsgFillPayload( struct msghdr * pxMsgh, uint8_t * pucBuffer, size_t uxLength )
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

void vAncillaryMsgFreePayload( struct msghdr * pxMsgh )
{
    /* Note: the caller must check if the array is empty */

    for( BaseType_t uxIter = 0; uxIter < pxMsgh->msg_iovlen; ++uxIter )
    {
        vPortFree( pxMsgh->msg_iov[ uxIter ].iov_base );
    }

    vPortFree( pxMsgh->msg_iov );
}

BaseType_t xAncillaryMsgControlFill( struct msghdr * pxMsgh, struct cmsghdr * pxCmsgVec, void ** ppvDataVec, size_t * puxDataLenVec, size_t uxNumBuffers )
{
    struct cmsghdr * pxCmsghIter;
    size_t uxTotalSpace = 0;

    if( pxMsgh == NULL )
    {
        return pdFALSE;
    }

    for( size_t uxIter = 0; uxIter < uxNumBuffers; ++uxIter )
    {
        uxTotalSpace += CMSG_SPACE( puxDataLenVec[ uxIter ] );
    }

    /* Note: the address returned by portMalloc is already aligned by 8 bytes */
    uint8_t * pxBuffer = pvPortMalloc( uxTotalSpace );
    if( pxBuffer == NULL )
    {
            pxMsgh->msg_control = NULL;
            pxMsgh->msg_controllen = 0;
            return pdFAIL;
    }

    pxMsgh->msg_control = pxBuffer;
    pxMsgh->msg_controllen = uxTotalSpace;

    pxCmsghIter = CMSG_FIRSTHDR( pxMsgh );

    for( size_t uxIter = 0; uxIter < uxNumBuffers; ++uxIter )
    {
        if( pxCmsghIter == NULL )
        {
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

BaseType_t xAncillaryMsgControlFillSingle( struct msghdr * pxMsgh, struct cmsghdr * pxCmsg, void * pvData, size_t puxDataLen )
{
    return xAncillaryMsgControlFill( pxMsgh, pxCmsg, &pvData, &puxDataLen, 1);
}

void vAncillaryMsgFreeControl( struct msghdr * pxMsgh )
{
    vPortFree( pxMsgh->msg_control );
}
