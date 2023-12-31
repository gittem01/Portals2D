#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <box2d/box2d.h>
#include <vector>
#include <set>
#include <map>

class AnimStruct
{
public:
    float* value;
    AnimStruct* prev;
    AnimStruct* next;
    AnimStruct* chainedAnimations;
    AnimStruct* lastChainedAimation;

    void (*registeredFunction)(float xProg, float magnitude, float from, float* value);

    float xProg;
    float scale;

    float from;
    float magnitude;

    static void linearFunc(float xProg, float magnitude, float from, float* value);
};

class AnimGroup
{
public:
    AnimGroup** toNull = nullptr;
    AnimGroup* next = nullptr;
    AnimGroup* prev = nullptr;
    // allocation list
    AnimStruct* animStructs = nullptr;
    AnimStruct* pointerHeader = nullptr;
    AnimStruct* lastFreePointer = nullptr;
    int count = 0;

    AnimGroup(int count=1);
    ~AnimGroup();
    
    void updateStructWithPointer(float* valuePtr, float scale, float from, float to);
    AnimStruct* removeStruct(AnimStruct* animStruct);
    AnimStruct* createChainedAnimation(AnimStruct* animStruct, float* value, float scale, float from = 0.0f, float to = 1.0f);
    AnimStruct* createAnimation(float* value, float scale, float from = 0.0f, float to = 1.0f);
};

// per fixture
typedef struct
{
    std::vector<std::vector<uint32_t>> dynamicOffsets;
    struct s_buffers* vertexBuffer;
    struct PBFixtureData* pbfData;
    b2Shape::Type renderType;
} s_debug_renderStruct;

// per fixture
typedef struct
{
    std::vector<s_debug_renderStruct*> renderFixtures;
    class PortalBody* portalBody;
} s_debug_renderBody;

// per worldIndex
typedef struct
{
    // ordered consistently with the fixture list
    std::vector<s_buffers*> vertexBuffers;
    std::vector<s_buffers*> vertexBuffersMirrored;
    // available only for polygon fixtures
    // data in between polygons are irrelevant
    std::vector<int> vertexStorageOffsets;
    std::vector<int> vertexStorageOffsetsMirrored;

    std::vector<b2Vec2> circlePositionings;
    std::vector<b2Vec2> circlePositioningsMirrored;

    std::set<s_debug_renderBody*> renderBodies;
} s_debug_renderGroup;

typedef struct renderOffsets
{
    uint32_t bodyOffsets;
    uint32_t fixtureOffsets;
} renderOffsets;


// per fixture
typedef struct
{
    std::vector<struct PBFixtureData*> renderFixtures;
    class PortalBody* portalBody;
} s_renderBody;

// per worldIndex
typedef struct
{
    std::vector<VkDescriptorSet*> imageSets;
    // ordered consistently with the fixture list
    std::vector<s_buffers*> vertexBuffers;
    std::set<s_renderBody*> renderBodies;

    int currentImgIndex;
} s_renderGroup;

typedef struct PortalRender
{
    class Portal* portal;
    AnimGroup* animGroup;
    s_portalPsh portalPsh;
    s_portalParticlePsh particlePsh;
    glm::vec2 scale;
} s_portalRender;

// per fixture
typedef struct
{
    std::vector<std::array<uint32_t, 2>> dynamicOffsets;
    s_buffers* vertexBuffer;
    s_buffers* indexBuffer;
    struct PBFixtureData* pbfData;
    VkDescriptorSet imageDescriptorSet;
} s_renderStruct3D;

// per fixture
typedef struct
{
    std::vector<uint32_t> dynamicOffsets;
    std::vector<s_renderStruct3D*> renderFixtures;
    class PortalBody* portalBody;
} s_renderBody3D;

// per worldIndex
typedef struct RenderGroup
{
    glm::mat4 scaleMatrix;
    std::vector<struct MeshData*> meshDatas;
    std::set<s_renderBody3D*> renderBodies;
} s_renderGroup3D;

typedef struct PortalRender3D
{
    std::vector<uint32_t> dynamicOffsets;
    AnimGroup* animGroup;
    float portalSizeX;
    float portalSizeY;
    float portalSizeUp;
    float sideFlatness;
    float noiseMult;
    float noiseScale;
} s_portalRender3D;