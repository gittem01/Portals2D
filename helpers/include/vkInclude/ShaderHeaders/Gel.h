#pragma once

#include <ShaderBase.h>

typedef struct
{
    glm::vec4 color;
    glm::vec4 fData; // gridSize, none, none, none
    glm::ivec4 iData; // numGridX, numGridY, numGels, none
} s_gelData;

typedef struct
{
    glm::vec4 posR; // posx, posy, radius, none
} s_perGelData;

typedef struct
{
    uint16_t gels[64];
} s_gelSpatialData;

class Gel : public ShaderBase
{
public:
    Gel(ThinDrawer* thinDrawer) : ShaderBase(thinDrawer) { }

    std::vector<s_perGelData> basePositions;

    void prepareVertexData();

    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet();
};