#pragma once

#include <glm/glm.hpp>
#include <box2d/box2d.h>
#include <vector>
#include <PortalBody.h>

class Portal;
class fixtureData;

typedef enum GEL_TYPE_N : char
{
    NONE = 0x1,
    BLUE = 0x2,
    ORANGE = 0x4,
    WHITE = 0x8,
    CLEAR = 0x10,
} GEL_TYPE;

typedef enum WALL_TYPE_N : char
{
    PORTALLABLE = 0x1,
    NOT_PORTALLABLE = 0x2,
    NOT_PORTALLABLE_2 = 0x4, // can't portal at all
    NOT_PORTALLABLE_4 = 0x8, // can't portal at all
} WALL_TYPE;

typedef enum GameDataType
{
    DEFAULT_OBJECT_O = 0x1 << 0,
    DEFAULT_CUBE_O = 0x1 << 1,
    LASER_CUBE_O = 0x1 << 2,
    SPHERE_CUBE_O = 0x1 << 3,
    TURRET_O = 0x1 << 4,
    PLAYER_O = 0x1 << 5,
    GEL_O = 0x1 << 6,
} g_DataType;

#define PICKABLES (DEFAULT_CUBE_O | LASER_CUBE_O | SPHERE_CUBE_O | TURRET_O)

enum FixtureDataType
{
    F_OTHER         = 0x1 << 0,
    F_PORTAL        = 0x1 << 1, 
    F_PORTAL_BODY   = 0x1 << 2,
    F_EDGE          = 0x1 << 3,
    F_GEDGE         = 0x1 << 4,
    F_FEDGE         = 0x1 << 5,

    F_END           = 0x1 << 30,
};

typedef class fixtureData
{
public:
    void* data = nullptr;
    void* extraData = nullptr;
    FixtureDataType type;

    fixtureData()
    {
        extraData = nullptr;
        type = F_END;
    }

} fixtureData;

typedef struct UnitWallData
{
    GEL_TYPE gelType = NONE;
    WALL_TYPE wallType = PORTALLABLE;
    float gelAmount = 0.0f;
} s_unitWallData;

const int unitWallPerMeter = 10;

typedef struct EdgeGlobalData
{
    std::vector<s_unitWallData> unitWalls;
    b2Vec2 localPoints[2];
    EdgeGlobalData* sideEdges[2];
    b2Vec2 dir;
    b2Vec2 perpDir;
    float totalSize;
    float perWallSize;
    int renderOfsset;

    EdgeGlobalData(b2Vec2 p0, b2Vec2 p1)
    {
        this->localPoints[0] = p0;
        this->localPoints[1] = p1;
        this->dir = p1 - p0;
        this->dir.Normalize();
        this->perpDir = b2Vec2(dir.y, -dir.x);
        this->totalSize = (p1 - p0).Length();

        this->unitWalls.resize(std::round(totalSize * unitWallPerMeter));
        this->perWallSize = totalSize / unitWalls.size();

        this->sideEdges[0] = nullptr;
        this->sideEdges[1] = nullptr;
    }
} s_edgeGlobalData;

typedef struct EdgeLocalData
{
    b2Fixture* prevNextFixtures[2];
    s_edgeGlobalData* globalData;
    int globalRange[2];

    EdgeLocalData(s_edgeGlobalData* globalData)
    {
        this->globalData = globalData;
        
        this->globalRange[0] = 0;
        this->globalRange[1] = globalData->unitWalls.size() - 1;
        
        this->prevNextFixtures[0] = nullptr;
        this->prevNextFixtures[1] = nullptr;
    }
} s_edgeLocalData;

typedef struct
{
    g_DataType type;
    void* data;
} g_PhysicsData;

b2Body* createEdge(b2Vec2 p0, b2Vec2 p1, PortalWorld* world);
b2Body* createEdgeOneSided(b2Vec2 p0, b2Vec2 p1, b2Vec2 p2, b2Vec2 p3, FixtureDataType type, PortalWorld* world, b2Body* body=nullptr);
b2Body* createBasicEdge(b2Vec2 p0, b2Vec2 p1, FixtureDataType type, PortalWorld* world);
b2Body* createBasicEdgeOneSided(b2Vec2 p0, b2Vec2 p1, b2Vec2 p2, b2Vec2 p3, FixtureDataType type, PortalWorld* world);
