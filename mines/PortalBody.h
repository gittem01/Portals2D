#pragma once

#include <box2d/box2d.h>
#include <vector>
#include <set>
#include <map>
#include <GLFW/glfw3.h>

class PortalBody{
public:
    static std::vector<PortalBody*> portalBodies;

    std::map<b2Body*, std::vector<void*>*> bodyMaps;
    b2World* world;
    b2Vec3 bodyColor;

    PortalBody(b2Body* body, b2World* world, b2Vec3 bodyColor=b2Vec3(1.0f, 0.0f, 1.0f));

    // fix1 is always a fixture of this class
    void collisionBegin(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2);
    void collisionEnd(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2);
    void preCollision(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2);

    void drawBodies();

    // draw fixtures according to the portal collision status
    void drawPolygonFix(b2Fixture* fixture);
    void drawCircleFix(b2Fixture* fixture);

    void drawVertices(b2Body* body, b2Vec2* vertices, int vertexCount);

    b2Vec2 getLineIntersection(b2Vec2 p1, b2Vec2 p2, b2Vec2 p3, b2Vec2 p4);
};