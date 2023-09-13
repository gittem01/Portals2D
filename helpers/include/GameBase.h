#pragma once

#include <glm/glm.hpp>
#include <box2d/box2d.h>
#include <vector>

class Portal;
class fixtureData;

typedef enum GEL_TYPE_N : char
{
    NONE = 0x1,
    BLUE = 0x2,
    ORANGE = 0x4,
    WHITE = 0x8,
} GEL_TYPE;

typedef enum WALL_TYPE_N : char
{
    PORTALLABLE = 0x1,
    NOT_PORTALLABLE = 0x2,
} WALL_TYPE;

typedef enum
{
    DEFAULT_OBJECT = 0x1 << 0,
} g_DataType;

typedef struct UnitWallData
{
    GEL_TYPE gelType = NONE;
    WALL_TYPE wallType = PORTALLABLE;
    char gelAmount = 0;
} s_unitWallData;

const int unitWallPerMeter = 10;

typedef struct EdgeGlobalData
{
    std::vector<s_unitWallData> unitWalls;
    b2Vec2 localPoints[2];
    EdgeGlobalData* sideEdges[2];
    b2Vec2 dir;
    float totalSize;
    float perWallSize;

    EdgeGlobalData(b2Vec2 p0, b2Vec2 p1)
    {
        this->localPoints[0] = p0;
        this->localPoints[1] = p1;
        this->dir = p1 - p0;
        this->dir.Normalize();
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