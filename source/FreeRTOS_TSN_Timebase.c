#include "FreeRTOS.h"

#include "FreeRTOS_TSN_Timebase.h"

#define NS_IN_ONE_SEC ( 1000000000UL )

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


BaseType_t xTimespecSum( struct freertos_timespec * pxOut, struct freertos_timespec * pxOp1, struct freertos_timespec * pxOp2 )
{
	struct freertos_timespec xOp1 = *pxOp1, xOp2 = *pxOp2;

	pxOut->tv_sec = xOp1.tv_sec + xOp2.tv_sec;
	pxOut->tv_nsec = ( xOp1.tv_nsec + xOp2.tv_nsec ) % NS_IN_ONE_SEC;

	if( pxOut->tv_nsec < xOp1.tv_nsec || pxOut->tv_nsec < xOp2.tv_nsec )
	{
		++pxOut->tv_sec;
	}

	if( pxOut->tv_sec < xOp1.tv_sec || pxOut->tv_sec < xOp2.tv_sec )
	{	
		return pdFAIL;
	}

	return pdPASS;
}

BaseType_t xTimespecDiff( struct freertos_timespec * pxOut, struct freertos_timespec * pxOp1, struct freertos_timespec * pxOp2 )
{
	pxOut->tv_sec = pxOp1->tv_sec - pxOp2->tv_sec;

	if( pxOp1->tv_nsec >= pxOp2->tv_nsec )
	{
		pxOut->tv_nsec = pxOp1->tv_nsec - pxOp2->tv_nsec;
	}
	else
	{	
		pxOut->tv_nsec = NS_IN_ONE_SEC - ( pxOp2->tv_nsec - pxOp1->tv_nsec );
		--pxOut->tv_sec;
	}

	return pdPASS;
}

BaseType_t xTimespecDiv( struct freertos_timespec * pxOut, struct freertos_timespec * pxOp1, BaseType_t xOp2 )
{
	struct freertos_timespec xOp1 = *pxOp1, xTemp;

	pxOut->tv_sec = xOp1.tv_sec / xOp2;
	pxOut->tv_nsec = xOp1.tv_nsec / xOp2;

	for( int i = 0; i < xOp1.tv_sec % xOp2; ++i )
	{
		xTemp.tv_sec = 0;
		xTemp.tv_nsec = NS_IN_ONE_SEC / xOp2;
		xTimespecSum( pxOut, pxOut, &xTemp );
	}

	return pdPASS;
}

BaseType_t xTimespecCmp( struct freertos_timespec * pxOp1, struct freertos_timespec * pxOp2 )
{
	if( pxOp1->tv_sec > pxOp2->tv_sec )
	{
		return 1;
	}
	else if( pxOp1->tv_sec == pxOp2->tv_sec )
	{
		if( pxOp1->tv_nsec > pxOp2->tv_nsec )
		{
			return 1;
		}
		else if( pxOp1->tv_nsec == pxOp2->tv_nsec )
		{
			return 0;
		}
		else
		{
			return -1;
		}
	}
	else
	{
		return -1;
	}
}
