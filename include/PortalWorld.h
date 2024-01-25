#pragma once

#include <box2d/box2d.h>
#include <vector>
#include <stdio.h>
#include <algorithm>
#include <vector>
#include <map>
#include <set>
#include <glm/glm.hpp>

struct portalConnection;
struct RotationJointDef;
struct PositionJointDef;

class PortalBody;
class Portal;
class PortalWorld;
class RotationJoint;
class PositionJoint;

enum ContactType
{
    BEGIN_CONTACT = 1,
    END_CONTACT = 2,
};

enum BodyDataType
{
	B_OTHER         = 0x1 << 0,
	B_PORTAL_BODY   = 0x1 << 1,
    B_EDGE          = 0x1 << 2,


	B_MOUSE         = 0x1 << 30,
};

typedef struct rayData
{
    b2Fixture* fix;
    b2Vec2 startPos;
    b2Vec2 endPos;
    b2Vec2 dir;
    Portal* portal;
} rayData;

typedef struct bodyData
{
	BodyDataType type;
	void* data;
    void* extraData;
} bodyData;

typedef struct
{
    PortalBody* pBody;
    Portal* collPortal;
    int side;
} bodyStruct;

typedef struct point
{
    glm::vec2 pos;
    float r;
} point;

typedef struct
{
    std::vector<PortalBody*> pbIndices;
    
    std::vector<b2Shape*> shapes;
    std::vector<b2Shape*> mirroredShapes;

    std::vector<point> outerVertices;
    std::vector<glm::vec4> outerEdges;
    std::vector <float> angles;

    std::vector<point> mirroredOuterVertices;
    std::vector<glm::vec4> mirroredOuterEdges;
    std::vector <float> mirroredAngles;

    b2Vec2 massCenter;
    b2Vec2 mirroredMassCenter;

    std::vector<std::pair<void (*) (PortalBody*, void*), void*>> createCallbacks;
    std::vector<std::pair<void (*) (PortalBody*, void*), void*>> destroyCallbacks;
} pbWorldData;

class PortalRay : public b2RayCastCallback
{
public:
    PortalRay(PortalWorld* pWorld);
    
    void sendRay_i(b2Vec2 rayStart, b2Vec2 dirVec, float rayLength, int rayIndex, Portal* rayOutPortal);
    void prepareRaySend();

    PortalWorld* pWorld;
    float minFraction = 1.0f;

    int maxRayCount = 30;

    // ray fraction of the portal midFixture (collision point on the midFixture)
    float portalFraction = 1.0f;
    float leftLength = 0.0;

    Portal* closestPortal;
    b2Fixture* closestFixture;
    b2Fixture* closestPortalFixture;
    void* filterBody;

    std::vector<rayData> rayResult;

    void sendRay(b2Vec2 rayStart, b2Vec2 dirVec, float rayLength, void* filterBody=nullptr, int rayIndex=0, Portal* rayOutPortal=nullptr);

    void reset();
    void endHandle();

    float ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float fraction);
};

class PortalWorld : public b2World
{

friend class TestPlayer;
friend class PortalBody;
friend class Portal;
friend class PortalWorldQuery;
friend class PortalRay;
friend class b2World;
friend class ContactListener;
friend class PositionJoint;
friend class PositionJointDef;
friend class RotationJoint;
friend class RotationJointDef;
friend class ThinDrawer;
friend class TestCase8;
friend class Leveler;

public:
    std::vector<pbWorldData*> bodyIndices;
    std::vector<Portal*> portals;

    PortalWorld();
    ~PortalWorld();

    void PortalStep(float timeStep, int32 velocityIterations, int32 positionIterations);

    Portal* createPortal(b2Vec2 pos, b2Vec2 dir, float size, b2Body* onBody=nullptr);
    void destroyPortal(Portal* portal);

    PortalBody* createPortalBody(b2Body* body);

    std::vector<void (*) (Portal*)> portalCreateCallbacks;
    std::vector<void (*) (Portal*)> portalDestroyCallbacks;

    PortalRay* rayHandler;

    static b2Vec2 mirror(b2Vec2 mirror, b2Vec2 vec);
    //world and local
    static b2Vec2 getRelativePortalPosition_W(b2Vec2 point, portalConnection* pConnect);
    static b2Vec2 getRelativePortalPositionReverse_W(b2Vec2 point, portalConnection* pConnect);
    static b2Vec2 getRelativePortalPosition_L(b2Vec2 point, portalConnection* pConnect);
    static b2Vec2 getRelativePortalPositionReverse_L(b2Vec2 point, portalConnection* pConnect);

private:
    Portal* createPortalWithChecks(b2Fixture* edgeFix, b2Vec2 pos, float size);

    std::set<PortalBody*> destroyBodies;
    std::set<pbWorldData*> handledBodies;

    static float normalizeAngle(float angle);
    static b2Vec2 rotateVec(b2Vec2 vec, float angle);
    static bool isLeft(b2Vec2 a, b2Vec2 b, b2Vec2 c, float t);
    static float vecAngle(b2Vec2 v1, b2Vec2 v2);
    static float getDist(b2Vec2 a, b2Vec2 b, b2Vec2 c);
    static float getRealDist(b2Vec2 a, b2Vec2 b, b2Vec2 c);
    static b2Vec2 project(b2Vec2 a, b2Vec2 b);

    static float calcAngle2(b2Vec2 vec);
    static void normalize(b2Vec2* vec);
    static b2Vec2 getLineIntersection(b2Vec2 p1, b2Vec2 p2, b2Vec2 p3, b2Vec2 p4);

    static glm::vec4 tangenter(point p1, point p2, float mult);
    static void calculateOuterEdges(std::vector<b2Shape*> shapes, std::vector<point>& outerVertices, std::vector<glm::vec4>& outerEdges, std::vector<float>& angles);
    static float getDistanceFromPortal(Portal* portal, PortalBody* portalBody);

    void globalPostHandle();

    bool createCloneBody(Portal* collPortal, int side, PortalBody* pBody);
    void connectBodies(PortalBody* body1, PortalBody* body2, portalConnection* connection);

    // baseBody should not be NULL if isNew is false
    void createNewPortalBody_i(PortalBody* pBody);
    void createPortalBody_i(PortalBody* pBody, PortalBody* baseBody);

    void CreateRotationJoint(RotationJointDef* def, PortalBody* pb1, PortalBody* pb2);
    void DestroyCustomJoint(b2Joint* cj);

    void CreatePositionJoint(PositionJointDef* def);

    void commonJointCreation(b2Joint* cj);
};