#pragma once

#include "PortalBody.h"
#include "PortalWorld.h"

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
};

class Portal;

struct portalConnection {
    int side1;
    int side2;
    int isReversed;
    Portal* portal1;
    Portal* portal2;
};

class Portal{

friend Portal* PortalWorld::createPortal(b2Vec2 pos, b2Vec2 dir, float size);

private:
    Portal(b2Vec2 pos, b2Vec2 dir, float size, PortalWorld* pWorld);

public:

    DebugDrawer* drawer;

    PortalWorld* pWorld;
    b2Body* body;
    b2Fixture* midFixture;
    b2Fixture* collisionSensor;
    b2Fixture* yFix[2];

    b2RayCastInput rcInp1;
    b2RayCastInput rcInp2;

    std::set<b2Fixture*> collidingFixtures[2];
    std::set<b2Fixture*> releaseFixtures[2];

    std::vector<portalConnection*> connections[2];

    b2Vec2 points[2];
    b2Vec2 pos;
    b2Vec2 dir;
    float size;
    float angle;
    b2Color color;
    int id;

    ~Portal();
    void clear();

    int getPointSide(b2Vec2 point);

    // fix1 is always a fixture of the portal
    int collisionBegin (b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2);
    int collisionEnd   (b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2);
    int preCollision   (b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2);

    void postHandle();

    void calculatePoints();
    void createPortalBody();

    void draw();
    void connect(Portal* portal2, int side1=0, int side2=0, int isReversed=0);

    void connectBodies(b2Body* body1, b2Body* body2);

    int getFixtureSide(b2Fixture* fix);
    int handleCollidingFixtures(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2);
    bool isCollisionProper(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2);
    bool rayCheck(b2Fixture* fix);

    bool shouldCollide(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2, portalCollision* coll);

    std::vector<b2Vec2> getCollisionPoints(b2Fixture* fix1, b2Fixture* fix2);

    std::vector<b2Vec2> collideCircleCircle(b2Fixture* fix1, b2Fixture* fix2);
    std::vector<b2Vec2> collidePolygonOther(b2Fixture* fix1, b2Fixture* fix2, b2Fixture* cFix, b2Fixture* oFix);
    std::vector<b2Vec2> collideEdgeOther(b2Fixture* fix1, b2Fixture* fix2);
    b2Vec2 getFixtureCenter(b2Fixture* fix);

    b2Vec2 getRayPoint(b2RayCastInput& input, b2RayCastOutput& output);
};