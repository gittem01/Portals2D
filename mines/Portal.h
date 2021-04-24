#pragma once

#include <box2d/box2d.h>
#include "Shape.h"
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


class Portal
{
private:

public:
    static std::set<Portal*> portals;

    b2Body* portalBody;
    b2Fixture* midFixture;
    b2Fixture* collisionSensor;
    b2Fixture* yFix[2];

    b2PolygonShape bottomShape;

    std::set<b2Fixture*> collidingFixtures;
    std::set<b2Body*> destroyQueue;
    std::set<b2Fixture*> prepareFixtures;
    std::vector<Shape*> addShapes;
    std::vector<b2Body*> addBodies;

    std::map<b2Body*, b2Body*> correspondingBodies;

    b2Vec2 points[2];
    b2Vec2 pos;
    b2Vec2 dir;
    float size;
    float angle;

    int id;

    Portal* connectedPortal;
    b2Color color;

    Portal(b2Vec2 pos, b2Vec2 dir, float size, b2World* world);
    ~Portal();

    void calculatePoints();
    void createPhysicalBody(b2World* world);
    void handleCollision(b2Fixture* fix1, b2Fixture* fix2, b2Contact* contact, contactType type);
    void handlePreCollision(b2Fixture* fixture, b2Contact* contact, const b2Manifold* oldManifold);
    bool shouldCollide(b2WorldManifold wManifold, int numOfPoints, int mode);
    void update();
    void draw();
    void connect(Portal* portal2);

    void connectBodies(b2Body* body1, b2Body* body2);
};