#pragma once

#include <box2d/box2d.h>
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

class EvaluationRay : public b2RayCastCallback
{

private:
    
    PortalWorld* pWorld;
    b2Fixture* closestFixture_i{};
    float minFraction_i = 1.0f;

public:
    std::vector<std::pair<b2Fixture*, b2Vec2>> hitFixtures;

    b2Fixture* closestFixture{};
    float minFraction = 1.0f;
    b2Vec2 rayNormal;

    EvaluationRay(PortalWorld* pWorld);
    void rayCast(b2Vec2 point1, b2Vec2 point2);
    float ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float fraction);
};

class PortalRay : public b2RayCastCallback
{

friend class PortalWorld;

private:
    PortalRay(PortalWorld* pWorld);
    
    void sendRay_i(b2Vec2 rayStart, b2Vec2 dirVec, float rayLength, int rayIndex, Portal* rayOutPortal);
    void prepareRaySend();

    PortalWorld* pWorld;
    float minFraction = 1.0f;

    int maxRayCount = 30;

    // ray fraction of the portal midFixture (collision point on the midFixture)
    float portalFraction = 1.0f;

    b2Fixture* closestFixture{};
    b2Fixture* closestPortalFixture{};

    std::vector<rayData*> rayResult;

    // portals can be on top of static bodies, and this
    // should resolve portal ray hitting issue in that case
    // if smallestFraction + portalThold < portalFraction then portal hit
    float portalThold = 0.0001f;
    
    void sendRay(b2Vec2 rayStart, b2Vec2 dirVec, float rayLength, int rayIndex=0, Portal* rayOutPortal=NULL);

    void reset();
    void endHandle();
    bool portalOutCheck(Portal* portal, b2Vec2 rayStart, b2Vec2 dirVec);

    float ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float fraction);
};

class PortalWorld : public b2World
{

friend class TestPlayer;
friend class PortalBody;
friend class Portal;
friend class PortalRay;
friend class b2World;
friend class ContactListener;

public:
    bool drawReleases;
    b2Color releaseColor;
    void drawUpdate();

    std::vector<std::vector<PortalBody*>*> bodyIndices;

    PortalWorld(DebugDrawer* drawer);
    void PortalStep(float timeStep, int32 velocityIterations, int32 positionIterations);

    Portal* createPortal(b2Vec2 pos, b2Vec2 dir, float size);
    PortalBody* createPortalBody(b2Body* body, b2Color bodyColor=b2Color(1.0f, 1.0f, 1.0f, 0.5f));

private:
    DebugDrawer* drawer;
    PortalRay* rayHandler;
    EvaluationRay* evalRayHandler;

    std::vector<PortalBody*> portalBodies;
    std::vector<Portal*> portals;
    std::set<PortalBody*> destroyBodies;

    static bool isLeft(b2Vec2& a, b2Vec2& b, b2Vec2& c, float t);
    static float vecAngle(b2Vec2 v1, b2Vec2 v2);
    static float getDist(b2Vec2& a, b2Vec2& b, b2Vec2& c);
    static float calcAngle2(b2Vec2 vec);
    static void normalize(b2Vec2* vec);
    static b2Vec2 mirror(b2Vec2 mirror, b2Vec2 vec);

    void globalPostHandle();

    std::vector<PortalBody*> createCloneBody(bodyStruct* s);
    void connectBodies(PortalBody* body1, PortalBody* body2, portalConnection* connection, int side, float angleRot);

    // baseBody should not be NULL if isNew is false
    void createPortalBody_i(PortalBody* pBody, PortalBody* baseBody, bool isNew);

    void CreateRotationJoint(b2PrismaticJointDef* def, bool isReversed, PortalBody* pb1, PortalBody* pb2, float angleRot);
};