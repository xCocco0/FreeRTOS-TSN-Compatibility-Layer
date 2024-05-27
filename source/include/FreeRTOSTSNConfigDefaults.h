
#ifndef FREERTOS_TSN_CONFIG_DEFAULTS_H
#define FREERTOS_TSN_CONFIG_DEFAULTS_H

#ifndef FREERTOS_TSN_CONFIG_H
    #error FreeRTOSTSNConfig.h has not been included yet
#endif

#define tsnconfigENABLE ( 1 )

#define tsnconfigDISABLE ( 0 )

/*
 *
 */
#ifndef tsnconfigDEFAULT_QUEUE_TIMEOUT
	#define tsnconfigDEFAULT_QUEUE_TIMEOUT ( 50U )
#endif

#if ( tsnconfigDEFAULT_QUEUE_TIMEOUT < 0 )
	#error tsnconfigDEFAULT_QUEUE_TIMEOUT must be a non negative integer
#endif

/*
 *
 */
#ifndef tsnconfigMAX_QUEUE_NAME_LEN	
	#define tsnconfigMAX_QUEUE_NAME_LEN 32U
#endif

#if ( tsnconfigMAX_QUEUE_NAME_LEN < 0 )
	#error tsnconfigMAX_QUEUE_NAME_LEN must be a non negative integer
#endif

/*
 *
 */
#ifndef tsnconfigINCLUDE_QUEUE_EVENT_CALLBACKS	
	#define tsnconfigINCLUDE_QUEUE_EVENT_CALLBACKS tsnconfigDISABLE
#endif

#if ( ( tsnconfigINCLUDE_QUEUE_EVENT_CALLBACKS != tsnconfigDISABLE ) && ( tsnconfigINCLUDE_QUEUE_EVENT_CALLBACKS != tsnconfigENABLE ) )
	#error Invaalid tsnconfigINCLUDE_QUEUE_EVENT_CALLBACKS configuration
#endif

/*------*/

/*
 *
 */
#ifndef tsnconfigNAME	
	#define tsnconfigNAME tsnconfigDISABLE
#endif

#if ( ( tsnconfigNAME != tsnconfigDISABLE ) && ( tsnconfigNAME != tsnconfigENABLE ) )
	#error Invaalid tsnconfigNAME configuration
#endif

#endif /* FREERTOS_TSN_CONFIG_DEFAULTS_H */
