#include <GameBase.h>
#include <PortalBody.h>

void createEdgeData(fixtureData** fData, g_EdgeData** eData)
{
	fixtureData* fixData = (fixtureData*)malloc(sizeof(fixtureData));
	g_PhysicsData* phyData = (g_PhysicsData*)malloc(sizeof(g_PhysicsData));
	g_EdgeData*	edgData = (g_EdgeData*)malloc(sizeof(g_EdgeData));

	fixData->extraData = (void*)phyData;
	phyData->type = EDGE;
	phyData->data = edgData;

	*fData = fixData;
	*eData = edgData;
}