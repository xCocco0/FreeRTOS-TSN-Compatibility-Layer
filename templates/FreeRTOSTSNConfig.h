/**
 * @file FreeRTOSTSNConfig.h
 * @brief Configuratio file for TSN Compatibility layer project
 *
 * This file contains the parameters for configuring the TSN compatibility
 * layer.
 * Omitting a definition will use the default value.
 * For in depth documentation, please check FreeRTOSTSNConfigDefaults.h
 */

#ifndef FREERTOS_TSN_CONFIG_H
#define FREERTOS_TSN_CONFIG_H

#define tsnconfigDEFAULT_QUEUE_TIMEOUT            ( 50U )
#define tsnconfigCONTROLLER_MAX_EVENT_WAIT        ( 1000U )
#define tsnconfigMAX_QUEUE_NAME_LEN               ( 32U )
#define tsnconfigINCLUDE_QUEUE_EVENT_CALLBACKS    tsnconfigDISABLE
#define tsnconfigUSE_PRIO_INHERIT                 tsnconfigDISABLE
#define tsnconfigTSN_CONTROLLER_PRIORITY          ( configMAX_PRIORITIES - 1 )
#define tsnconfigCONTROLLER_HAS_DYNAMIC_PRIO      tsnconfigDISABLE
#define tsnconfigWRAPPER_INSERTS_VLAN_TAGS        tsnconfigENABLE
#define tsnconfigSOCKET_INSERTS_VLAN_TAGS         tsnconfigDISABLE
#define tsnconfigERRQUEUE_LENGTH                  ( 16 )
#define tsnconfigDUMP_PACKETS                     tsnconfigDISABLE

#endif /* FREERTOS_TSN_CONFIG_H */
