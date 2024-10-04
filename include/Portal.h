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

class PortalWorldQuery : public b2QueryCallback
{
public:
    Portal* portal;

    PortalWorldQuery(Portal* portal);

	virtual bool ReportFixture(b2Fixture* fixture);
};

class Portal
{

friend class PortalWorld;
friend class PortalBody;
friend class PortalRay;
friend class PortalWorldQuery;
friend class PositionJoint;
friend class RotationJoint;
friend class TestPlayer;
friend class Gel;
friend class Platform;

public:

    void connect(Portal* portal2, bool isReversed=false, int side1=0, int side2=0);
    void setVoid(int side);

    std::vector<portalConnection*> connections[2];

    b2Vec2 points[2];
    bool isVoid[2];
    b2Vec2 pos;
    b2Vec2 dir;
    float dirAngle;
    b2Vec2 rotDir;
    float size;

    int range[2];
    b2Fixture* sideFixtures[2];
    PortalWorldQuery* queryClass;
    b2Vec2 boxMin;
    b2Vec2 boxMax;

    void reviveAround();

    bool isParallel(b2Vec2 p1, b2Vec2 p2);
    void setLinVel(b2Vec2 speed);
    b2Vec2 getLinVel();
    int getPointSide(b2Vec2 point);
    int getPointSideVertical(b2Vec2 point);

    void setAngVel(float angVel);
    float getAngVel();
    b2Body* body;

private:

    Portal(b2Vec2 pos, b2Vec2 dir, float size, PortalWorld* pWorld, b2Body* onBody);

    b2Vec2 posOffset;
    float angleOffset;

    PortalWorld* pWorld;
    
    b2Fixture* midFixture;
    b2Fixture* yFix[2];

    b2RayCastInput rcInp1;
    b2RayCastInput rcInp2;

    std::set<PortalBody*> collidingBodies[2];

    ~Portal();

    b2Fixture* getPreDestructionFixture();

    int getFixtureSide(b2Fixture* fix);

    // fix1 is always a fixture of the portal
    PortalCollisionType collisionBegin (b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2);
    PortalCollisionType collisionEnd   (b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2);
    PortalCollisionType preCollision   (b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2);

    void calculatePoints();
    void createPortalBody(b2Body* onBody);

    PortalCollisionType handleCollidingFixtures(b2Contact* contact, b2Fixture* fix2);
    bool rayCheck(b2Fixture* fix);

    bool shouldCollide(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2, portalCollision* coll1, portalCollision* coll2);
    
    static b2Vec2 getFixtureCenter(b2Fixture* fix);
    static b2Vec2 getRayPoint(b2RayCastInput& input, b2RayCastOutput& output);
};