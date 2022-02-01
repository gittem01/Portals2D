#pragma once

#include "DebugDrawer.h"
#include "PortalWorld.h"
#include <box2d/box2d.h>
#include <vector>
#include <set>
#include <map>
#include <GLFW/glfw3.h>

struct bodyData;
struct portalConnection;

class Portal;
class PortalBody;
class PortalWorld;

typedef struct {
    Portal* portal;
    int status; // 1 for colliding 0 for released
    int side;
}portalCollision;

typedef struct{
    PortalBody* body;
    portalConnection* connection;
}bodyCollisionStatus;


class PortalBody{
    
friend PortalBody* PortalWorld::createPortalBody(b2Body* body, b2Color bodyColor); 
friend std::vector<PortalBody*> PortalWorld::createCloneBody(bodyStruct* s);

private:
    PortalBody(b2Body* body, PortalWorld* pWorld, b2Color bodyColor=b2Color(1.0f, 1.0f, 1.0f, 0.5f));

public:

    ~PortalBody();

    std::vector<PortalBody*>* worldIndex;

    std::vector<bodyCollisionStatus*>* bodyMaps;
    std::map<b2Fixture*, std::set<portalCollision*>*> fixtureCollisions;
    std::map<b2Fixture*, std::set<Portal*>> prepareMaps;
    std::vector<bodyStruct*> createBodies;

    // index 0 reserved for drawVertices
    // remaining vertices are release vertices
    std::map<b2Fixture*, std::vector<std::vector<b2Vec2>*>*> allParts;

    b2Body* body;
    b2Color bodyColor;

    PortalWorld* pWorld;

    // fix1 is always a fixture of this class
    void collisionBegin(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2);
    void collisionEnd(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2);
    void preCollision(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2);

    void destroyCheck(b2Fixture* fix, Portal* portal);
    bool shouldCreate(b2Body* body, Portal* portal, int side);
    bool shouldCollide(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2, bodyData* bData);
    void outHelper(b2Fixture* fix, Portal* portal, int status, int side);
    void handleOut(b2Fixture* fix, Portal* portal, int out);

    float getArea(b2Fixture* fix, int status);
    b2Vec2 getCenterOfMass(b2Fixture* fix, int status);
    void calculateParts(b2Fixture* fix);

    void adjustVertices(std::vector<b2Vec2>* vertices, std::vector<b2Vec2>* retVertices1,
                        std::vector<b2Vec2>* retVertices2, Portal* portal, int side);
    void drawBodies();
    void portalRender(b2Fixture* fix, std::vector<b2Vec2>& vertices);
    // draw fixtures according to the portal collision status
    void drawPolygonFix(b2Fixture* fixture);
    void drawCircleFix(b2Fixture* fixture);
    void drawVertices(b2Body* body, std::vector<b2Vec2>& vertices);

    std::vector<PortalBody*> postHandle();

    b2Vec2 getLineIntersection(b2Vec2 p1, b2Vec2 p2, b2Vec2 p3, b2Vec2 p4);
};