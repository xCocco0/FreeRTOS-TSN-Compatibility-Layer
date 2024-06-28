# FreeRTOS TSN Compatibility Layer

The FreeRTOS TSN Compatibility Layer is an additional component working alongside FreeRTOS and its FreeRTOS-Plus-TCP addon. The purpose of this project is to extend the features provided by the Plus TCP addon in order to provide better support over TSN networks. This comprises:
- **The possibility of employing a multi-level queuing scheduler for scheduling packets, completely integrating Plus TCP's Network Event Queue**: instead of using one single queue, with the provided API the user can specify a generalized queue hierarchy starting from simple building blocks. The user can match the packets to queues by assigning filtering policies to each queue.
- **Support for VLAN tagging and Differentiated services**: by tuning the socket options it is possible to enable the insertion of VLAN tags in the Ethernet header and setting the Differentiated services code in the IP header for sent packets
- **Support for control messages over sockets**: this is a mechanism similar to Linux ancillary messages, allowing to add/retrieve additional information to sent/received packets. This also allows the user to generate timestamps for packets, using an API that is similar to the Linux one.

## Setting up the project

This project requires FreeRTOS and FreeRTOS Plus TCP addons, without any modification required. In order to provide the given functionalities, this library should act as an intermediary between FreeRTOS Plus TCP and the Network Interface. In order to do that, we created a wrapper for the network interface that hijacks the traffic towards out implementation. This means that the previous NetworkInterface.c **should not be compiled** with the sources, and NetworkWrapper.c should be compiled in its place. Remember that the chosen NetworkInterface.c should as well be present in the include list, with its dependencies.

If you are having troubles with setting up the project, you can find an example in [this repository](https://github.com/xCocco0/freertos-tcp-nucleo144/blob/0311196a9e2e8b5424ef50ecabacafc984284742/Core/Src/netqueues.c) using Makefile.

## Configuration

The project allows configuration at different levels:

### Network scheduler

The network scheduler is a tree like structure that manages the order in which packets are served. The root of the tree is where the scheduling starts from, and the leaves of the trees are the queues holding the packets.\
The network scheduler queues should be specified by defining the ``vNetworkQueueInit`` function in the user project. You can find a template in the ``templates/`` directory and a working example [here](https://github.com/xCocco0/freertos-tcp-nucleo144/tree/TSN).\
The signature of the function is:
```c
void vNetworkQueueInit( void );
```
It has the duty of allocating schedulers and queues, linking them and assigning the scheduler root by calling ``xNetworkQueueAssignRoot()``.\
Queues are created by calling ``pxNetworkQueueCreate()``, and has type ``NetworkQueue_t``, schedulers has type ``NetworkNode_t`` instead, and are created using a functions that are specific for each scheduler.\
If a scheduler admits only one children, it is possibile to link a queue to it using ``xNetworkSchedulerLinkQueue()``. To link another scheduler, ``xNetworkSchedulerLinkChild()`` should be used.\
Please note that it is not possible to link both a scheduler and a queue to the same scheduler. Consider creating a FIFO scheduler before and linking the queue to the FIFO and the FIFO to the previous scheduler.

If the user wants to create his custom schedulers, ``FreeRTOS_TSN_NetworkSchedulerBlock.h`` provides an useful API that allows to do so. Also check the example in ``templates/``.

### Timebase

In order to give a better estimate of the timing, the user should specify the timebase that is used for acquiring timestamps.\
The interfacing with the timebase should be defined by the user with the project sources. A function
```c
void vTimebaseInit( void )
```
should be defined, creating a ``TimebaseHandle_t`` object, assigning the required functions and calling ``xTimebaseHandleSet()``. Please note that this function is also expected to start the timebase.

You can see an example of configuration for an STM32 board [here](https://github.com/xCocco0/freertos-tcp-nucleo144/blob/0311196a9e2e8b5424ef50ecabacafc984284742/Core/Src/timebase.c).

### TSN Config

As with FreeRTOS and FreeRTOS-Plus-TCP a configuration file must be provided by the user. You can start from the template in ``templates/`` directory and find more details for the single parameters in the default configuration file ``FreeRTOSTSNConfigDefaults.h``.

## TSN Sockets

Whereas the scheduling functions are shared with sockets in FreeRTOS Plus TCP, some features have required the definition of a socket extension, that we call _TSN Sockets_. The API to use this sockets is the same used for the Plus TCP sockets, but includes these additional capabilities:
- Setting the VLAN tag and DSCP socket options for the packets being sent by this socket
- Enabling timestamping for received or sent packets.
- Using recvmsg() to retrieve a packet together with its ancillary control data.

An example of usage can be found [here](https://github.com/xCocco0/freertos-tcp-nucleo144/tree/TSN).
