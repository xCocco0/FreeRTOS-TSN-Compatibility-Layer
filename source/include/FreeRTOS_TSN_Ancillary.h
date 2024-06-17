#ifndef FREERTOS_TSN_ANCILLARY_H
#define FREERTOS_TSN_ANCILLARY_H

#include "FreeRTOS.h"

#include "FreeRTOS_IP.h"

struct iovec         /* Scatter/gather array items */
{
    void * iov_base; /* Starting address */
    size_t iov_len;  /* Number of bytes to transfer */
};

struct msghdr
{
    void * msg_name;        /* optional address */
    socklen_t msg_namelen;  /* size of address */
    struct iovec * msg_iov; /* scatter/gather array */
    size_t msg_iovlen;      /* # elements in msg_iov */
    void * msg_control;     /* ancillary data, see below */
    size_t msg_controllen;  /* ancillary data buffer len */
    int msg_flags;          /* flags on received message */
};

struct cmsghdr
{
    socklen_t cmsg_len; /* data byte count, including header */
    int cmsg_level;     /* originating protocol */
    int cmsg_type;      /* protocol-specific type */
    /* followed by unsigned char cmsg_data[]; */
};

struct sock_extended_err
{
    uint32_t ee_errno; /* Error number */
    uint8_t ee_origin; /* Where the error originated */
    uint8_t ee_type;   /* Type */
    uint8_t ee_code;   /* Code */
    uint8_t ee_pad;    /* Padding */
    uint32_t ee_info;  /* Additional information */
    uint32_t ee_data;  /* Other data */
    /* More data may follow */
};

#ifndef pdFREERTOS_ERRNO_ENOMSG
/* This is somehow missing from projdefs.h */
    #define pdFREERTOS_ERRNO_ENOMSG    42
#endif

#define SO_EE_ORIGIN_NONE              0
#define SO_EE_ORIGIN_LOCAL             1
#define SO_EE_ORIGIN_ICMP              2
#define SO_EE_ORIGIN_ICMP6             3
#define SO_EE_ORIGIN_TXSTATUS          4
#define SO_EE_ORIGIN_ZEROCOPY          5
#define SO_EE_ORIGIN_TXTIME            6
#define SO_EE_ORIGIN_TIMESTAMPING      SO_EE_ORIGIN_TXSTATUS

#define CMSG_ALIGN( len )    ( ( ( len ) + sizeof( long ) - 1 ) & ~( sizeof( long ) - 1 ) )

#define CMSG_DATA( cmsg )    ( ( void * ) ( ( char * ) ( cmsg ) + CMSG_ALIGN( sizeof( struct cmsghdr ) ) ) )

#define CMSG_SPACE( len )    ( CMSG_ALIGN( sizeof( struct cmsghdr ) ) + CMSG_ALIGN( len ) )

#define CMSG_LEN( len )      ( CMSG_ALIGN( sizeof( struct cmsghdr ) ) + ( len ) )

#define __CMSG_FIRSTHDR( ctl, len )         \
    ( ( len ) >= sizeof( struct cmsghdr ) ? \
      ( struct cmsghdr * ) ( ctl ) :        \
      ( struct cmsghdr * ) NULL )

#define CMSG_FIRSTHDR( msg )    __CMSG_FIRSTHDR( ( msg )->msg_control, ( msg )->msg_controllen )

struct cmsghdr * __CMSG_NXTHDR( void * ctl,
                                size_t size,
                                struct cmsghdr * cmsg );

#define CMSG_NXTHDR( mhdr, cmsg )    __CMSG_NXTHDR( ( mhdr )->msg_control, ( mhdr )->msg_controllen, ( cmsg ) )


struct msghdr * pxAncillaryMsgMalloc();

void vAncillaryMsgFree( struct msghdr * pxMsgh );

void vAncillaryMsgFreeAll( struct msghdr * pxMsgh );

BaseType_t xAncillaryMsgFillName( struct msghdr * pxMsgh,
                                  IP_Address_t * xAddr,
                                  uint16_t usPort,
                                  BaseType_t xFamily );

void vAncillaryMsgFreeName( struct msghdr * pxMsgh );

BaseType_t xAncillaryMsgFillPayload( struct msghdr * pxMsgh,
                                     uint8_t * pucBuffer,
                                     size_t uxLength );

void vAncillaryMsgFreePayload( struct msghdr * pxMsgh );

BaseType_t xAncillaryMsgControlFill( struct msghdr * pxMsgh,
                                     struct cmsghdr * pxCmsgVec,
                                     void ** pvDataVec,
                                     size_t * puxDataLenVec,
                                     size_t uxNumBuffers );

BaseType_t xAncillaryMsgControlFillSingle( struct msghdr * pxMsgh,
                                           struct cmsghdr * pxCmsg,
                                           void * pvData,
                                           size_t puxDataLen );

void vAncillaryMsgFreeControl( struct msghdr * pxMsgh );

#endif /* FREERTOS_TSN_ANCILLARY_H */
