/**
 * @file SchedulerTemplate.h
 * @brief Template for creating new schedulers
 *
 * This file contains a dummy implementation to use as a reference for
 * defining new schedulers.
 */

#ifndef SCHED_YOURNAME_H
#define SCHED_YOURNAME_H

#include "FreeRTOS_TSN_NetworkSchedulerBlock.h"

/** @brief Function to create this scheduler
 * This function will allocate a new scheduler and setup
 * its parameters accordingly
 */
NetworkNode_t * pxNetworkNodeCreateYourName( UBaseType_t uxParam1, UBaseType_t uxParam2 );

#endif /* ifndef SCHED_YOURNAME_H */
