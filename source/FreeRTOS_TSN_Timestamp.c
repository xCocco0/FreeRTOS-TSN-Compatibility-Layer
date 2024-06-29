/**
 * @file FreeRTOS_TSN_Timestamp.c
 * @brief Implementation of the timestamping features
 */

#include "FreeRTOS_TSN_Timestamp.h"
#include "FreeRTOS_TSN_Timebase.h"

/**
 * @brief Acquires the software timestamp.
 *
 * This function acquires the software timestamp by suspending all tasks, getting the time from the timebase,
 * and then resuming all tasks.
 *
 * @param ts Pointer to the freertos_timespec structure where the acquired timestamp will be stored.
 */
void vTimestampAcquireSoftware( struct freertos_timespec * ts )
{
    vTaskSuspendAll();
    vTimebaseGetTime( ts );
    ( void ) xTaskResumeAll();
}
