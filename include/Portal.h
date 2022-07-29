#pragma once

#include "PortalBody.h"

class Portal;

typedef struct portalConnection {
    Portal* portal1;
    Portal* portal2;
    int side1;
    int side2;

    bool isReversed;
} portalConnection;

class Portal{

friend class PortalWorld;
friend class PortalBody;
friend class PortalRay;

public:

    void connect(Portal* portal2, bool isReversed=false, int side1=0, int side2=0);
    void setVoid(int side);

    std::vector<portalConnection*> connections[2];

    // will be a private variable later
    std::map<b2Fixture*, int> releaseFixtures[2];

    b2Vec2 points[2];
    bool isVoid[2];
    b2Vec2 pos;
    b2Vec2 dir;
    float size;

    void evaluateWorld();

private:

    Portal(b2Vec2 pos, b2Vec2 dir, float size, PortalWorld* pWorld);
    std::set<b2Fixture*> extraFixtures;

    static std::map<b2Fixture*, b2Vec2> noCollData;

    PortalWorld* pWorld;
    b2Body* body;
    b2Fixture* midFixture;
    b2Fixture* collisionSensor;
    b2Fixture* yFix[2];

    b2RayCastInput rcInp1;
    b2RayCastInput rcInp2;

    std::set<b2Fixture*> prepareFixtures;
    std::set<b2Fixture*> collidingFixtures[2];

    ~Portal();

    int getPointSide(b2Vec2 point);
    int getFixtureSide(b2Fixture* fix);

    // fix1 is always a fixture of the portal
    PortalCollisionType collisionBegin (b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2);
    PortalCollisionType collisionEnd   (b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2);
    PortalCollisionType preCollision   (b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2);

    void calculatePoints();
    void createPortalBody();

    PortalCollisionType handleCollidingFixtures(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2);
    bool rayCheck(b2Fixture* fix);

    bool shouldCollide(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2, portalCollision* coll);
    bool prepareCollisionCheck(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2);

    std::vector<b2Vec2> getUsableRayPoints(b2Fixture* fix, int side);

    // always returns 2 point
    b2Vec2* getMaxRayPoints(std::vector<b2Vec2> usableRayPoints, int side);
    bool isPointIn(b2Vec2 p1, b2Vec2 p2, b2Vec2 point, int side, float sideThold=-0.001f);
    
    std::vector<b2Vec2> getCollisionPoints(b2Fixture* fix1, b2Fixture* fix2);

    static std::vector<b2Vec2> collideCircleCircle(b2Fixture* fix1, b2Fixture* fix2);
    static std::vector<b2Vec2> collidePolygonOther(b2Fixture* fix1, b2Fixture* fix2, b2Fixture* cFix, b2Fixture* oFix);
    static std::vector<b2Vec2> collideEdgeOther(b2Fixture* fix1, b2Fixture* fix2);

    static b2Vec2 getFixtureCenter(b2Fixture* fix);
    static b2Vec2 getRayPoint(b2RayCastInput& input, b2RayCastOutput& output);
};