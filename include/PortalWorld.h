#pragma once

#include <box2d/box2d.h>
#include <vector>
#include <stdio.h>
#include <algorithm>
#include <vector>
#include <map>
#include <set>
#include <unordered_set>

typedef enum{
    BEGIN_CONTACT = 1,
    END_CONTACT = 2,
} contactType;

enum dataTypes
{
	OTHER = 0,
	PORTAL = 1,
	PORTAL_BODY = 2,
	MOUSE = 3,
};

struct bodyData {
	dataTypes type;
	void* data;

    // reserved for user
    void* extraData;
};

struct portalConnection;

class PortalBody;
class Portal;
class DebugDrawer;
class PortalWorld;

typedef struct{
    PortalBody* pBody;
    Portal* collPortal;
    int side;
}bodyStruct;

// ray goes through portals
class PortalRay : public b2RayCastCallback
{
public:
    PortalWorld* pWorld;
    float minFraction = 1.0f;

    // ray fraction of the portal midFixture
    float portalFraction = 1.0f;

    b2Fixture* closestFixture;
    b2Fixture* closestPortalFixture;

    // portals can be on top of static bodies, and this
    // should resolve portal ray hitting issue in that case
    // if smallestFraction + portalThold < portalFraction then portal hit
    float portalThold = 0.0001f;

    PortalRay(PortalWorld* pWorld);
    
    void reset();
    void endHandle();

    float ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float fraction);
};

class PortalWorld{

private:
    int maxRayCount = 20;

    DebugDrawer* drawer;
    PortalRay* rayHandler;

    std::vector<PortalBody*> portalBodies;
    std::vector<Portal*> portals;
    std::set<PortalBody*> destroyBodies;

    bool isLeft(b2Vec2& a, b2Vec2& b, b2Vec2& c, float t);
    float vecAngle(b2Vec2 v1, b2Vec2 v2);
    float getDist(b2Vec2& a, b2Vec2& b, b2Vec2& c);
    float calcAngle2(b2Vec2 vec);
    void normalize(b2Vec2* vec);
    b2Vec2 mirror(b2Vec2 mirror, b2Vec2 vec);

    // baseBody should not be NULL if isNew is false
    void createPortalBody_i(PortalBody* pBody, PortalBody* baseBody, bool isNew);

public:
    bool drawReleases;
    b2Color releaseColor;

    b2World* world;

    std::vector<std::vector<PortalBody*>*> bodyIndices;

    PortalWorld(b2World* world, DebugDrawer* drawer);

    std::vector<PortalBody*> createCloneBody(bodyStruct* s);
    void connectBodies(b2Body* body1, b2Body* body2, portalConnection* connection, int side);

    void portalUpdate();
    void globalPostHandle();
    void drawUpdate();

    void sendRay(b2Vec2 rayStart, b2Vec2 dirVec, float rayLength, int rayIndex=0);

    Portal* createPortal(b2Vec2 pos, b2Vec2 dir, float size);
    PortalBody* createPortalBody(b2Body* body, b2Color bodyColor=b2Color(1.0f, 1.0f, 1.0f, 0.5f));

    friend class TestPlayer;
    friend class PortalBody;
    friend class Portal;
    friend class PortalRay;
};