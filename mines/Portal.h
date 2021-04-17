#pragma once

#include <box2d/box2d.h>
#include "polygon.h"
#include <vector>
#include <stdio.h>
#include <algorithm>
#include <vector>
#include <map>
#include <set>

typedef enum{
    BEGIN_CONTACT = 1,
    END_CONTACT = 2,
} contactType;


class Portal
{
private:

public:
    static std::set<Portal*> portals;

    b2Body* portalBody;
    b2Fixture* midFixture;
    b2Fixture* yFix[2];

    std::set<b2Body*> destroyQueue;
    std::set<b2Fixture*> newFixtures;
    std::vector<polygon*> addPolygons;

    std::map<b2Fixture*, std::set<b2Contact*>> unhandledCollisions;

    b2Vec2 points[2];
    b2Vec2 pos;
    b2Vec2 dir;
    float size;
    float angle;

    int id;

    std::set<b2Fixture*> collidingFixtures;

    Portal* connectedPortal;
    b2Color color;

    Portal(b2Vec2 pos, b2Vec2 dir, float size, b2World* world);
    ~Portal();

    void calculatePoints();
    void createPhysicalBody(b2World* world);
    void handleCollision(b2Fixture* fix1, b2Fixture* fix2, b2Contact* contact, contactType type);
    void handlePreCollision(b2Fixture* fixture, b2Contact* contact, const b2Manifold* oldManifold);
    bool shouldCollide(b2WorldManifold wManifold, int numOfPoints);
    void update();
    void draw();
    void connect(Portal* portal2);

    void onContact(b2Body* body);
    void newBody(b2Body* body);
    bool isColliding(b2Fixture* fixture);
};