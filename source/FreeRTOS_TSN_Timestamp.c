#include "FreeRTOS_TSN_Timestamp.h"
#include "FreeRTOS_TSN_Timebase.h"

void vTimestampAcquireSoftware( struct freertos_timespec * ts )
{	
	vTaskSuspendAll();
    vTimebaseGetTime( ts );
	( void ) xTaskResumeAll();
}
