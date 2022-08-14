#pragma once

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

enum PortalCollisionType
{
    // status type : BEFORE_AFTER_SIDE
    NONE_COLLIDING_0 = 1 << 0,
    NONE_COLLIDING_1 = 1 << 1,

    COLLIDING_NONE_0 = 1 << 2,
    COLLIDING_NONE_1 = 1 << 3,

    COLLIDING_RELEASE_0 = 1 << 4,
    COLLIDING_RELEASE_1 = 1 << 5,

    RELEASE_COLLIDING_0 = 1 << 6,
    RELEASE_COLLIDING_1 = 1 << 7,

    // For release values greater than 1
    E_RELEASE_COLLIDING_0 = 1 << 8,
    E_RELEASE_COLLIDING_1 = 1 << 9,

    E_COLLIDING_RELEASE_0 = 1 << 10,
    E_COLLIDING_RELEASE_1 = 1 << 11,

    DEFAULT_COLLISION = 1 << 31,
};

typedef struct
{
    Portal* portal;
    int status; // 1 for colliding 0 for released
    int side;
} portalCollision;

typedef struct
{
    std::vector<portalCollision*> pCollisions;
    void* extraData;
} fixtureData;

typedef struct
{
    PortalBody* body;
    portalConnection* connection;
} bodyCollisionStatus;


class PortalBody
{
    
friend class PortalWorld;
friend class ContactListener;

public:
    std::vector<PortalBody*>* worldIndex;
    std::vector<bodyCollisionStatus*>* bodyMaps;

    b2Body* body;
    int numFixtures;
    float offsetAngle;
    bool isMirrored;

private:
    PortalBody(b2Body* body, PortalWorld* pWorld);

    ~PortalBody();

    std::vector<bodyStruct*> createBodies;
    std::map<Portal*, int> outFixtures[2];

    std::vector<float>* baseDensities;

    PortalWorld* pWorld;

    // fix1 is always a fixture of this class
    void collisionBegin(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2);
    void collisionEnd(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2);
    void preCollision(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2);

    void destroyCheck(b2Fixture* fix, Portal* portal, PortalCollisionType out);
    bool shouldCreate(b2Body* body, Portal* portal, PortalCollisionType out);
    bool shouldCollide(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2, bodyData* bData);
    void outHelper(b2Fixture* fix, Portal* portal, int status, PortalCollisionType out);
    void handleOut(b2Fixture* fix, Portal* portal, PortalCollisionType out);
    void releaseOut2(b2Fixture* fix, Portal* portal);

    void postHandle();
    void applyPartialGravity(b2Fixture* fixture);
    
    static b2Vec2 findPolygonCentroid(b2Vec2* vertices, int numVertices);
    static float findPolygonArea(b2Vec2* vertices, int numVertices);
};