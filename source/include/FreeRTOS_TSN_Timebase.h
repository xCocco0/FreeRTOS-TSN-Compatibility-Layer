#ifndef FREERTOS_TSN_TIMEBASE_H
#define FREERTOS_TSN_TIMEBASE_H

#include "FreeRTOS.h"
#include "task.h"

struct freertos_timespec
{
    uint32_t tv_sec;
    uint32_t tv_nsec;
};

typedef void ( * TimeBaseStartFunction_t )( void );

typedef void ( * TimeBaseStopFunction_t )( void );

typedef void ( * TimeBaseSetTimeFunction_t )( const struct freertos_timespec * ts );

typedef void ( * TimeBaseGetTimeFunction_t )( struct freertos_timespec * ts );

typedef enum
{
    eTimebaseNotInitialised = 0,
    eTimebaseDisabled,
    eTimebaseEnabled
} eTimebaseState_t;

struct xTIMEBASE
{
    TimeBaseStartFunction_t fnStart;
    TimeBaseStopFunction_t fnStop;
    TimeBaseSetTimeFunction_t fnSetTime;
    TimeBaseGetTimeFunction_t fnGetTime;
};

typedef struct xTIMEBASE TimebaseHandle_t;

extern void vTimebaseInit( void );

BaseType_t xTimebaseHandleSet( TimebaseHandle_t * pxTimebase );

void vTimebaseStart( void );

void vTimebaseStop( void );

void vTimebaseSetTime( struct freertos_timespec * ts );

void vTimebaseGetTime( struct freertos_timespec * ts );

BaseType_t xTimebaseGetState( void );


#endif /* FREERTOS_TSN_TIMEBASE_H */
