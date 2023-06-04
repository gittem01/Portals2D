#pragma once

#include <glm/glm.hpp>
#include <box2d/box2d.h>

class Portal;
class fixtureData;

typedef enum
{
    NONE = 0x01,
    BLUE = 0x02,
    ORANGE = 0x04,
    WHITE = 0x08,
} GEL_TYPE;

typedef enum
{
    PORTALLABLE = 0x01,
    NOT_PORTALLABLE = 0x02,
    PORTAL_ITSELF = 0x04,
} WALL_TYPE;

typedef enum
{
    DEFAULT_OBJECT = 0x01,
    EDGE = 0x02,
} g_DataType;

typedef struct edgeData
{
    struct edgeData* edges[2];
    b2Fixture* fix;
    GEL_TYPE gel;
    WALL_TYPE wall;
} g_EdgeData;

typedef struct
{
    g_DataType type;
    void* data;
} g_PhysicsData;

void createEdgeData(fixtureData** fData, g_EdgeData** eData);
g_EdgeData* createEdge(b2Vec2 p1, b2Vec2 p2, b2World* world, b2Body* body);
g_EdgeData* createEdge(struct createDestrData* cdData);
b2Vec2 getVertexByIndex(g_EdgeData* edgeData, int vertexNum);
void fillCdData(struct createDestrData* cdData, g_EdgeData* edgeData);