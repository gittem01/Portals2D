#pragma once

#include "DebugDrawer.h"
#include <box2d/box2d.h>
#include <vector>
#include <set>
#include <map>
#include <GLFW/glfw3.h>

struct bodyData;
class Portal;

typedef struct {
    Portal* portal;
    int status; // 0 for colliding 1 for released
    int side;
} portalCollision;

class PortalBody{
public:
    static std::vector<PortalBody*> portalBodies;
    static b2Color releaseColor;
    static bool drawReleases;

    std::map<b2Body*, std::vector<void*>*> bodyMaps;
    std::map<b2Fixture*, std::set<portalCollision*>*> fixtureCollisions;
    b2World* world;
    b2Color bodyColor;

    PortalBody(b2Body* body, b2World* world, b2Color bodyColor=b2Color(1.0f, 1.0f, 1.0f, 0.5f));

    // fix1 is always a fixture of this class
    void collisionBegin(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2);
    void collisionEnd(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2);
    void preCollision(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2);

    bool shouldCollide(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2, bodyData* bData);
    void outHelper(b2Fixture* fix, Portal* portal, int status, int side);
    void handleOut(b2Fixture* fix, Portal* portal, int out);

    void drawBodies();

    void adjustVertices(std::vector<b2Vec2>& vertices, std::vector<b2Vec2>& retVertices1,
                        std::vector<b2Vec2>& retVertices2, Portal* portal, int side);
    void portalRender(b2Fixture* fix, std::vector<b2Vec2>& vertices);

    // draw fixtures according to the portal collision status
    void drawPolygonFix(b2Fixture* fixture);
    void drawCircleFix(b2Fixture* fixture);

    void drawVertices(b2Body* body, std::vector<b2Vec2>& vertices);

    b2Vec2 getLineIntersection(b2Vec2 p1, b2Vec2 p2, b2Vec2 p3, b2Vec2 p4);
};