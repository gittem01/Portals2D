#include <GameBase.h>
#include <PortalBody.h>
#include <Portal.h>

void createEdgeData(fixtureData** fData, g_EdgeData** eData)
{
	fixtureData* fixData = (fixtureData*)malloc(sizeof(fixtureData));
	g_PhysicsData* phyData = (g_PhysicsData*)malloc(sizeof(g_PhysicsData));
	g_EdgeData*	edgData = (g_EdgeData*)malloc(sizeof(g_EdgeData));

	fixData->extraData = (void*)phyData;
	phyData->type = EDGE;
	phyData->data = edgData;

	edgData->edges[0] = nullptr;
	edgData->edges[1] = nullptr;

	*fData = fixData;
	*eData = edgData;
}

g_EdgeData* createEdge(b2Vec2 p1, b2Vec2 p2, b2World* world, b2Body* body)
{
    b2Vec2 v1 = body->GetLocalPoint(p1);
    b2Vec2 v2 = body->GetLocalPoint(p2);

    b2Vec2 dir = v1 - v2;
    b2EdgeShape shape;
    shape.SetOneSided(v1 + dir, v1, v2, v2 - dir);
    
    //shape.SetTwoSided(v1, v2);

    fixtureData* fData;
    g_EdgeData* eData;

    createEdgeData(&fData, &eData);

    b2FixtureDef fixDef;
    fixDef.userData.pointer = (uintptr_t)fData;
    fixDef.shape = &shape;
    b2Fixture* fix = body->CreateFixture(&fixDef);

    eData->fix = fix;

    return eData;
}

g_EdgeData* createEdge(createDestrData* cdData)
{
    b2Vec2 v1 = cdData->body->GetLocalPoint(cdData->vertexPositions[0]);
    b2Vec2 v2 = cdData->body->GetLocalPoint(cdData->vertexPositions[1]);
    
    b2Vec2 dir = v1 - v2;
    b2EdgeShape shape;
    shape.SetOneSided(v1 + dir, v1, v2, v2 - dir);
    
    //shape.SetTwoSided(v1, v2);

    fixtureData* fData;
    g_EdgeData* eData;

    createEdgeData(&fData, &eData);
    
    b2FixtureDef fixDef;
    fixDef.restitution = cdData->restitution;
    fixDef.friction = cdData->friction;
    fixDef.userData.pointer = (uintptr_t)fData;
    fixDef.shape = &shape;
    b2Fixture* fix = cdData->body->CreateFixture(&fixDef);

    eData->fix = fix;
    eData->gel = cdData->gel;
    eData->wall = cdData->wall;

    return eData;
}

b2Vec2 getVertexByIndex(g_EdgeData* edgeData, int vertexNum)
{
    b2Fixture* eFix = edgeData->fix;
    b2EdgeShape* eShape = (b2EdgeShape*)eFix->GetShape();
    if (vertexNum)
    {
        return eFix->GetBody()->GetWorldPoint(eShape->m_vertex2);
    }
    else
    {
        return eFix->GetBody()->GetWorldPoint(eShape->m_vertex1);
    }
}

void fillCdData(createDestrData* cdData, g_EdgeData* edgeData)
{
    cdData->body = edgeData->fix->GetBody();
    cdData->friction = edgeData->fix->GetFriction();
    cdData->restitution = edgeData->fix->GetRestitution();
    cdData->gel = edgeData->gel;
    cdData->wall = edgeData->wall;

    cdData->vertexPositions[0] = getVertexByIndex(edgeData, 0);
    cdData->vertexPositions[1] = getVertexByIndex(edgeData, 1);
}