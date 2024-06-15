
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
#ifndef tsnconfigCONTROLLER_HAS_DYNAMIC_PRIO
	#define tsnconfigCONTROLLER_HAS_DYNAMIC_PRIO tsnconfigDISABLE
#endif

#if ( ( tsnconfigCONTROLLER_HAS_DYNAMIC_PRIO != tsnconfigDISABLE ) && ( tsnconfigCONTROLLER_HAS_DYNAMIC_PRIO != tsnconfigENABLE ) )
	#error Invalid tsnconfigCONTROLLER_HAS_DYNAMIC_PRIO configuration
#endif


/* FreeRTOS priority of the TSN controller task. If tsnconfigCONTROLLER_HAS_DYNAMIC_PRIO
 * this config entry is ignored as the base priority of the TSN controller is
 * the priority of the idle task plus 1
 */
#ifndef tsnconfigTSN_CONTROLLER_PRIORITY
	#define tsnconfigTSN_CONTROLLER_PRIORITY ( configMAX_PRIORITIES - 1 )
#endif

#if ( ( tsnconfigTSN_CONTROLLER_PRIORITY < 0 ) )
	#error Invalid tsnconfigTSN_CONTROLLER_PRIORITY configuration
#endif

/* If the network interface has no support for adding VLAN tags to 802.1Q
 * packets, enabling this feature can be a turnaround for sending tagged
 * packets. Note that the effect of this option highly depends on the behaviour
 * of the lower levels (i.e. some MACs forcefully remove VLAN tags)
 */
#ifndef tsnconfigWRAPPER_INSERTS_VLAN_TAGS
	#define tsnconfigWRAPPER_INSERTS_VLAN_TAGS tsnconfigENABLE
#endif

#if ( ( tsnconfigWRAPPER_INSERTS_VLAN_TAGS != tsnconfigDISABLE ) && ( tsnconfigWRAPPER_INSERTS_VLAN_TAGS != tsnconfigENABLE ) )
	#error Invalid tsnconfigWRAPPER_INSERTS_VLAN_TAGS configuration
#endif

/* This option allows the user to set a flag in the sockets to specify the VLAN
 * tag. This option reduces compatibility with current +TCP sockets and makes
 * the API different from Linux sockets. This is going to be removed in future,
 * please consider using tsnconfigWRAPPER_INSERTS_VLAN_TAGS instead.
 */
#ifndef tsnconfigSOCKET_INSERTS_VLAN_TAGS
	#define tsnconfigSOCKET_INSERTS_VLAN_TAGS tsnconfigDISABLE
#endif

#if ( ( tsnconfigSOCKET_INSERTS_VLAN_TAGS != tsnconfigDISABLE ) && ( tsnconfigSOCKET_INSERTS_VLAN_TAGS != tsnconfigENABLE ) )
	#error Invalid tsnconfigSOCKET_INSERTS_VLAN_TAGS configuration
#endif

#if ( ( tsnconfigWRAPPER_INSERTS_VLAN_TAGS == tsnconfigENABLE ) && ( tsnconfigSOCKET_INSERTS_VLAN_TAGS == tsnconfigENABLE ) )
	#error tsnconfigWRAPPER_INSERTS_VLAN_TAGS and tsnconfigSOCKET_INSERTS_VLAN_TAGS cannot be enabled at the same time
#endif

/* The maximum number of messages waiting in a socket errqueue
 */
#ifndef tsnconfigERRQUEUE_LENGTH	
	#define tsnconfigERRQUEUE_LENGTH 16
#endif

#if ( tsnconfigERRQUEUE_LENGTH <= 0 )
	#error Invalid tsnconfigERRQUEUE_LENGTH configuration
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
