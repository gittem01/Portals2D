#pragma once

#include <RotationJoint.h>
#include <vector>
#include <stdio.h>
#include <algorithm>
#include <vector>
#include <map>
#include <set>

struct portalConnection;

class PortalBody;
class Portal;
class DebugDrawer;
class PortalWorld;

typedef enum
{
    BEGIN_CONTACT = 1,
    END_CONTACT = 2,
} contactType;

typedef enum
{
	OTHER = 0,
	PORTAL = 1,
	PORTAL_BODY = 2,
	MOUSE = 3,
} dataTypes;

typedef struct rayData
{
    b2Vec2 endPos;
    b2Vec2 dir;
} rayData;

typedef struct bodyData
{
	dataTypes type;
	void* data;

    // reserved for user
    void* extraData;
} bodyData;

typedef struct
{
    PortalBody* pBody;
    Portal* collPortal;
    int side;
} bodyStruct;

class PortalRay : public b2RayCastCallback
{
private:
    void sendRay_i(b2Vec2 rayStart, b2Vec2 dirVec, float rayLength, int rayIndex, Portal* rayOutPortal);
    void prepareRaySend();

public:
    PortalWorld* pWorld;
    float minFraction = 1.0f;

    int maxRayCount = 30;

    // ray fraction of the portal midFixture
    float portalFraction = 1.0f;

    b2Fixture* closestFixture{};
    b2Fixture* closestPortalFixture{};

    std::vector<rayData*> rayResult;

    // portals can be on top of static bodies, and this
    // should resolve portal ray hitting issue in that case
    // if smallestFraction + portalThold < portalFraction then portal hit
    float portalThold = 0.0001f;

    PortalRay(PortalWorld* pWorld);
    
    void sendRay(b2Vec2 rayStart, b2Vec2 dirVec, float rayLength, int rayIndex=0, Portal* rayOutPortal=NULL);

    void reset();
    void endHandle();
    bool portalOutCheck(Portal* portal, b2Vec2 rayStart, b2Vec2 dirVec);

    float ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float fraction);
};

class PortalWorld : public b2World{

private:
    DebugDrawer* drawer;
    PortalRay* rayHandler;

    std::vector<PortalBody*> portalBodies;
    std::vector<Portal*> portals;
    std::set<PortalBody*> destroyBodies;

    static bool isLeft(b2Vec2& a, b2Vec2& b, b2Vec2& c, float t);
    static float vecAngle(b2Vec2 v1, b2Vec2 v2);
    static float getDist(b2Vec2& a, b2Vec2& b, b2Vec2& c);
    static float calcAngle2(b2Vec2 vec);
    static void normalize(b2Vec2* vec);
    static b2Vec2 mirror(b2Vec2 mirror, b2Vec2 vec);

    // baseBody should not be NULL if isNew is false
    void createPortalBody_i(PortalBody* pBody, PortalBody* baseBody, bool isNew);

    b2Joint* CreateRotationJoint(b2PrismaticJointDef* def, bool isReversed);

public:
    bool drawReleases;
    b2Color releaseColor;

    std::vector<std::vector<PortalBody*>*> bodyIndices;

    PortalWorld(DebugDrawer* drawer);

    std::vector<PortalBody*> createCloneBody(bodyStruct* s);
    void connectBodies(b2Body* body1, b2Body* body2, portalConnection* connection, int side);

    void portalUpdate();
    void globalPostHandle();
    void drawUpdate();

    Portal* createPortal(b2Vec2 pos, b2Vec2 dir, float size);
    PortalBody* createPortalBody(b2Body* body, b2Color bodyColor=b2Color(1.0f, 1.0f, 1.0f, 0.5f));

    b2Joint* CreateRotationJoint(b2JointDef* def);

    friend class TestPlayer;
    friend class PortalBody;
    friend class Portal;
    friend class PortalRay;
    friend class b2World;
};