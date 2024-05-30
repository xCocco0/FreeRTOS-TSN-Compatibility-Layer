
#ifndef FREERTOS_TSN_CONFIG_DEFAULTS_H
#define FREERTOS_TSN_CONFIG_DEFAULTS_H

#ifndef FREERTOS_TSN_CONFIG_H
    #error FreeRTOSTSNConfig.h has not been included yet
#endif

#define tsnconfigENABLE ( 1 )

#define tsnconfigDISABLE ( 0 )

/* Queue timeout is used by the TSNController task for timeout on queue
 * operations. Note that if any queue is empty the TSN task will never
 * wait for a message on the single queue, but wait for an event for
 * tsnconfigCONTROLLER_MAX_EVENT_WAIT ticks.
 */
#ifndef tsnconfigDEFAULT_QUEUE_TIMEOUT
	#define tsnconfigDEFAULT_QUEUE_TIMEOUT ( 50U )
#endif

#if ( tsnconfigDEFAULT_QUEUE_TIMEOUT < 0 )
	#error tsnconfigDEFAULT_QUEUE_TIMEOUT must be a non negative integer
#endif

/* The TSN controller will wake up every time a packet is pushed on its queues
 * or until the next expected wakeup. This setting force the controller to
 * check queues again periodically, so that deadlocks are avoided in case the
 * user-defined schedulers have any issue.
 */
#ifndef tsnconfigCONTROLLER_MAX_EVENT_WAIT	
	#define tsnconfigCONTROLLER_MAX_EVENT_WAIT ( 1000U )
#endif

/* The max length for queue names. This number must take into account also the
 * space for the string terminator. A value of 0 means queue names are not used.
 */
#ifndef tsnconfigMAX_QUEUE_NAME_LEN	
	#define tsnconfigMAX_QUEUE_NAME_LEN 32U
#endif

#if ( tsnconfigMAX_QUEUE_NAME_LEN < 0 )
	#error tsnconfigMAX_QUEUE_NAME_LEN must be a non negative integer
#endif

/* Enable callbacks when a message is popped and pushed from any network
 * queue. The function prototype is defined in FreeRTOS_NetworkSchedulerQueue.h
 */
#ifndef tsnconfigINCLUDE_QUEUE_EVENT_CALLBACKS	
	#define tsnconfigINCLUDE_QUEUE_EVENT_CALLBACKS tsnconfigDISABLE
#endif

#if ( ( tsnconfigINCLUDE_QUEUE_EVENT_CALLBACKS != tsnconfigDISABLE ) && ( tsnconfigINCLUDE_QUEUE_EVENT_CALLBACKS != tsnconfigENABLE ) )
	#error Invalid tsnconfigINCLUDE_QUEUE_EVENT_CALLBACKS configuration
#endif

/* Used the IPV field in the network queue structure to implement a priority
 * inheritance mechanism on the network queues. Note that this is a simplified
 * version that assumes that whenever a packet is waiting in a queue, a task of
 * the corresponding priority is always waiting for it. The TSN controller will
 * then assume a priority which is the maximum IPV of all the queues with
 * pending messages.
 */
#ifndef tsnconfigUSE_PRIO_INHERIT
	#define tsnconfigUSE_PRIO_INHERIT tsnconfigDISABLE
#endif

#if ( ( tsnconfigUSE_PRIO_INHERIT != tsnconfigDISABLE ) && ( tsnconfigUSE_PRIO_INHERIT != tsnconfigENABLE ) )
	#error Invalid tsnconfigUSE_PRIO_INHERIT configuration
#endif

/*------*/

/*
 *
 */
#ifndef tsnconfigNAME	
	#define tsnconfigNAME tsnconfigDISABLE
#endif

#if ( ( tsnconfigNAME != tsnconfigDISABLE ) && ( tsnconfigNAME != tsnconfigENABLE ) )
	#error Invalid tsnconfigNAME configuration
#endif

#endif /* FREERTOS_TSN_CONFIG_DEFAULTS_H */
