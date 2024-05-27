
#include "BasicSchedulers.h"

/*----------------------------------------------------------------------------*/

struct xSCHEDULER_RR
{
	struct xSCHEDULER_GENERIC xScheduler;
};

// TODO

/*----------------------------------------------------------------------------*/

struct xSCHEDULER_PRIO
{
	struct xSCHEDULER_GENERIC xScheduler;
};

NetworkQueue_t * prvPrioritySelect( NetworkNode_t * pxNode )
{
	NetworkQueue_t * pxResult = NULL;

	for( uint16_t usIter = 0; usIter < pxNode->ucNumChildren; ++usIter )
	{
		pxResult = pxNetworkSchedulerCall( pxNode->pxNext[ usIter ] );
		if( pxResult != NULL )
		{
			break;
		}
	}

	return pxResult;
}

NetworkNode_t * pxNetworkNodeCreatePrio( BaseType_t uxNumChildren )
{

	NetworkNode_t *pxNode;
	struct xSCHEDULER_PRIO * pxSched;

	pxNode = pxNetworkNodeCreate( uxNumChildren );
	pxSched = ( struct xSCHEDULER_PRIO * ) pvNetworkSchedulerGenericCreate( pxNode, sizeof( struct xSCHEDULER_PRIO ) );

	pxSched->xScheduler.fnSelect = prvPrioritySelect;

	return pxNode;
}

/*----------------------------------------------------------------------------*/

struct xSCHEDULER_FIFO
{
	struct xSCHEDULER_GENERIC xScheduler;
};

NetworkNode_t * pxNetworkNodeCreateFIFO()
{

	NetworkNode_t *pxNode;
	struct xSCHEDULER_FIFO * pxSched;

	pxNode = pxNetworkNodeCreate( 1 );
	pxSched = ( struct xSCHEDULER_FIFO * ) pvNetworkSchedulerGenericCreate( pxNode, sizeof( struct xSCHEDULER_FIFO ) );

	( void ) pxSched;

	return pxNode;
}

/*----------------------------------------------------------------------------*/



