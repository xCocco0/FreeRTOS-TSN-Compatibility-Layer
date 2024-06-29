/**
 * @file SchedCBS.c
 * @brief Implementation of a Credit Based Scheduler.
 */
#include "SchedCBS.h"

struct xSCHEDULER_CBS
{
    struct xSCHEDULER_GENERIC xScheduler;
    UBaseType_t uxBandwidth; /*< bandwidth in bits per second */
    UBaseType_t uxMaxCredit; /*< max credit in bits
                              *  regulates burstiness */
    TickType_t uxNextActivation;
};

BaseType_t prvCBSReady( NetworkNode_t * pxNode )
{
    struct xSCHEDULER_CBS * pxSched = ( struct xSCHEDULER_CBS * ) pxNode->pvScheduler;
    TickType_t uxDelay, uxNow, uxMaxDelay;
    NetworkBufferDescriptor_t * pxNextPacket;
    BaseType_t xReturn;

    uxNow = xTaskGetTickCount();

    if( pxSched->uxNextActivation <= uxNow )
    {
        pxNextPacket = pxPeekNextPacket( pxNode );
        uxDelay = pdMS_TO_TICKS( ( pxNextPacket->xDataLength * 8 * 1000 ) / ( pxSched->uxBandwidth ) );
        uxMaxDelay = pdMS_TO_TICKS( ( pxSched->uxMaxCredit * 1000 ) / ( pxSched->uxBandwidth ) );

        if( pxSched->uxNextActivation + uxMaxDelay < uxNow )
        {
            pxSched->uxNextActivation = uxNow - uxMaxDelay;
        }

        pxSched->uxNextActivation += uxDelay;

        xReturn = pdTRUE;
    }
    else
    {
        vNetworkQueueAddWakeupEvent( pxSched->uxNextActivation );
        xReturn = pdFALSE;
    }

    return xReturn;
}

/** @brief Creates a CBS scheduler
 * @param uxBandwidth The desired bandwidth of the scheduler, measured in bit
 * per second
 * @param uxMaxCredit The max credit of the scheduler, in bits
 * @return A pointer to the node with the scheduler
 */
NetworkNode_t * pxNetworkNodeCreateCBS( UBaseType_t uxBandwidth,
                                        UBaseType_t uxMaxCredit )
{
    NetworkNode_t * pxNode;
    struct xSCHEDULER_CBS * pxSched;

    pxNode = pxNetworkNodeCreate( 1 );
    pxSched = ( struct xSCHEDULER_CBS * ) pvNetworkSchedulerGenericCreate( pxNode, sizeof( struct xSCHEDULER_CBS ) );

    pxSched->uxBandwidth = uxBandwidth;
    pxSched->uxMaxCredit = uxMaxCredit;
    pxSched->xScheduler.fnReady = prvCBSReady;
    pxSched->uxNextActivation = xTaskGetTickCount();

    return pxNode;
}
