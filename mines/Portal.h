#pragma once

#include <box2d/box2d.h>
#include <vector>
#include <stdio.h>
#include <algorithm>
#include <vector>
#include <map>
#include <set>
#include <unordered_set>
#include <PortalBody.h>

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
    Portal* portal;
};

class Portal
{
private:

public:
    static std::set<Portal*> portals;

    b2World* world;
    b2Body* body;
    b2Fixture* midFixture;
    b2Fixture* collisionSensor;
    b2Fixture* yFix[2];

    std::vector<b2Fixture*> prepareBegins[2];
    std::vector<b2Fixture*> prepareEnds[2];
    std::vector<b2Fixture*> collideBegins;
    std::vector<b2Fixture*> collideEnds;

    std::set<b2Fixture*> prepareFixtures[2];
    std::set<b2Fixture*> collidingFixtures[2];
    std::set<b2Fixture*> releaseFixtures[2];

    std::set<b2Fixture*> handleFixtures;

    std::vector<portalConnection*> connections;

    b2Vec2 points[2];
    b2Vec2 pos;
    b2Vec2 dir;
    float size;
    float angle;
    b2Color color;
    int id;

    Portal(b2Vec2 pos, b2Vec2 dir, float size, b2World* world);
    ~Portal();
    void clear();


    // fix1 is always a fixture of the portal
    void collisionBegin (b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2);
    void collisionEnd   (b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2);
    void preCollision   (b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2);

    void postHandle();

    void calculatePoints();
    void createPortalBody(b2World* world);

    void draw();
    void connect(Portal* portal2);

    void connectBodies(b2Body* body1, b2Body* body2);

    std::vector<b2Vec2> getCollisionPoints(b2Fixture*& fix1, b2Fixture*& fix2);

    std::vector<b2Vec2> collideCircleCircle(b2Fixture*& fix1, b2Fixture*& fix2);
    std::vector<b2Vec2> collidePolygonPolygon(b2Fixture*& fix1, b2Fixture*& fix2);
    std::vector<b2Vec2> collidePolygonOther(b2Fixture*& fix1, b2Fixture*& fix2);
    std::vector<b2Vec2> collideEdgeOther(b2Fixture*& fix1, b2Fixture*& fix2);

    b2Vec2 getRayPoint(b2RayCastInput& input, b2RayCastOutput& output);

    static void portalUpdate(){
        for (Portal* p : Portal::portals) {
            p->postHandle();
        }
    }
};