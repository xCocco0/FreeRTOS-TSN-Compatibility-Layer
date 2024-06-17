#include "FreeRTOS.h"

#include "FreeRTOS_TSN_Timebase.h"

static TimebaseHandle_t xTimebaseHandle;
static eTimebaseState_t xTimebaseState = eTimebaseNotInitialised;

BaseType_t xTimebaseHandleSet( TimebaseHandle_t * pxTimebase )
{
    /* For now, disallow changing the timebase handle on the fly */
    if( xTimebaseState != eTimebaseNotInitialised )
    {
        return pdFAIL;
    }

    if( ( pxTimebase->fnStart == NULL ) || ( pxTimebase->fnStop == NULL ) || ( pxTimebase->fnGetTime == NULL ) || ( pxTimebase->fnSetTime == NULL ) )
    {
        return pdFAIL;
    }

    xTimebaseHandle = *pxTimebase;
    xTimebaseState = eTimebaseEnabled;

    return pdPASS;
}

void vTimebaseStart()
{
    xTimebaseHandle.fnStart();
}

void vTimebaseSetTime( struct freertos_timespec * ts )
{
    xTimebaseHandle.fnSetTime( ts );
}

void vTimebaseGetTime( struct freertos_timespec * ts )
{
    xTimebaseHandle.fnGetTime( ts );
}

BaseType_t xTimebaseGetState()
{
    return xTimebaseState;
}
