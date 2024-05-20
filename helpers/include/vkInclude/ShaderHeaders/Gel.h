#pragma once

#include <ShaderBase.h>

#define NUM_GEL_TYPE 4
#define MAX_GEL_COUNT 4096
#define GEL_SQUARE_SIZE 2.0f

typedef struct
{
    glm::vec4 fData; // none, none, none, none
    glm::ivec4 iData; // swapchainImage, gelType, numSquares, start1
    glm::ivec4 iData2; // xSquareCount, ySquareCount, none, none
} s_gelData;

typedef struct
{
    glm::vec4 posR; // posx, posy, radius, none
    glm::vec4 color;
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
    s_perGelData preFillGelData[MAX_GEL_COUNT];
    s_gelSpatialData preFillSpatialData[32 * 32];
    glm::vec4 gelColors[NUM_GEL_TYPE] = {
        glm::vec4(0.0, 0.2, 1.0, 1.0),
        glm::vec4(1.0, 0.2, 0.0, 1.0),
        glm::vec4(0.6f, 0.6f, 0.6f, 1.0f),
        glm::vec4(0.05f, 0.05f, 0.05f, 0.2f)
    };

    int lastIndex;
    int xSquareCount;
    int ySquareCount;
    int requiredSquareSize;

    void preCalculate();
    void putGel(int gelType, glm::vec2 pos, float radius);
    void render(VkCommandBuffer cmdBuff, int swapChainImage);

    void prepareVertexData();

    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet();
};