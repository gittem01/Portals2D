#include <GameBase.h>
#include <PortalBody.h>
#include <Portal.h>

b2Body* createEdge(b2Vec2 p0, b2Vec2 p1, PortalWorld* world)
{
    b2BodyDef bDef;
    bDef.type = b2_staticBody;
    bDef.position = b2Vec2(0.0, 0.0);
    b2Body* body = world->CreateBody(&bDef);

    //b2Vec2 dir = p1 - p0;
    b2EdgeShape shape;
    //shape.SetOneSided(p0 - dir, p0, p1, p1 + dir);

    shape.SetTwoSided(p0, p1);

    fixtureData* fData = new fixtureData();
    fData->type = F_EDGE;

    s_edgeGlobalData* eGlobalData = new s_edgeGlobalData(p0, p1);
    s_edgeLocalData* eLocalData = new s_edgeLocalData(eGlobalData);

    fData->data = eLocalData;

    b2FixtureDef fixDef;
    fixDef.userData.pointer = (uintptr_t)fData;
    fixDef.shape = &shape;
    b2Fixture* fix = body->CreateFixture(&fixDef);

    return body;
}

b2Body* createBasicEdge(b2Vec2 p0, b2Vec2 p1, enum FixtureDataType type, PortalWorld* world)
{
    b2BodyDef bDef;
    bDef.type = b2_staticBody;
    bDef.position = b2Vec2(0.0, 0.0);
    b2Body* body = world->CreateBody(&bDef);

    //b2Vec2 dir = p1 - p0;
    b2EdgeShape shape;
    //shape.SetOneSided(p0 - dir, p0, p1, p1 + dir);

    shape.SetTwoSided(p0, p1);

    fixtureData* fData = new fixtureData();
    fData->type = type;

    b2FixtureDef fixDef;
    fixDef.userData.pointer = (uintptr_t)fData;
    fixDef.shape = &shape;
    b2Fixture* fix = body->CreateFixture(&fixDef);

    return body;
}