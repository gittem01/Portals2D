#pragma once

#include <ShaderBase.h>

#define NUM_GEL_TYPE 4
#define MAX_GEL_COUNT 2048
#define GEL_SQUARE_SIZE 2.0f

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
    glm::ivec4 gels[32];
} s_gelSpatialData;

class Gel : public ShaderBase
{
public:
    Gel(ThinDrawer* thinDrawer) : ShaderBase(thinDrawer) { }

    int numSquares;
    int spatialDataSize;
    int spatialDataLayerSize;

    std::vector<s_perGelData> basePositions;
    s_perGelData preFillGelData[NUM_GEL_TYPE][2048];
    s_gelSpatialData preFillSpatialData[NUM_GEL_TYPE][32 * 32];
    glm::vec4 gelColors[NUM_GEL_TYPE] = {
        glm::vec4(0.0, 0.3, 1.0, 1.0),
        glm::vec4(1.0, 0.3, 0.0, 1.0),
        glm::vec4(0.6f, 0.6f, 0.6f, 1.0f),
        glm::vec4(1.0f, 1.0f, 1.0f, 0.1f)
    };
    int lastIndices[NUM_GEL_TYPE] = {0, 0, 0, 0};
    int xSquareCount;
    int ySquareCount;
    int requiredSquareSize;

    void preCalculate();
    void putGel(int gelType, glm::vec2 pos, float radius);
    void renderGel(VkCommandBuffer cmdBuff, int gelType, int swapChainImage);

    void prepareVertexData();

    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet();
};