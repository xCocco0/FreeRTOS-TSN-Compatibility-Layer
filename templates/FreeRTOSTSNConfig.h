#ifndef FREERTOS_TSN_CONFIG_H
#define FREERTOS_TSN_CONFIG_H

#define tsnconfigDEFAULT_QUEUE_TIMEOUT            ( 50U )
#define tsnconfigCONTROLLER_MAX_EVENT_WAIT        ( 1000U )
#define tsnconfigMAX_QUEUE_NAME_LEN               32U
#define tsnconfigINCLUDE_QUEUE_EVENT_CALLBACKS    tsnconfigDISABLE
#define tsnconfigUSE_PRIO_INHERIT                 tsnconfigDISABLE
#define tsnconfigTSN_CONTROLLER_PRIORITY          ( configMAX_PRIORITIES - 1 )

#endif /* FREERTOS_TSN_CONFIG_H */
