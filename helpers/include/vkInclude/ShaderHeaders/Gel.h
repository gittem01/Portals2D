#pragma once

#include <ShaderBase.h>

#define NUM_GEL_TYPE 4
#define GEL_SQUARE_SIZE 2.0

typedef struct
{
    glm::vec4 color;
    glm::vec4 fData; // none, none, none, none
    glm::ivec4 iData; // swapchainImage, gelType, numSquares, start1
    glm::ivec4 iData2; // xSquareCount, ySquareCount, none, none
} s_gelData;

typedef struct
{
    glm::vec4 posR; // posx, posy, radius, none
} s_perGelData;

typedef struct
{
    glm::ivec4 gels[64];
} s_gelSpatialData;

class Gel : public ShaderBase
{
public:
    Gel(ThinDrawer* thinDrawer) : ShaderBase(thinDrawer) { }

    int numSquares;
    int spatialDataSize;
    int spatialDataLayerSize;

    std::vector<s_perGelData> basePositions;

    void prepareVertexData();

    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet();
};