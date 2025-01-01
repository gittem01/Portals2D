#pragma once

#include <WallBase.h>

typedef struct
{
    glm::vec4 posSize;
    glm::vec4 colour;
    glm::vec4 bgColour;
    glm::vec4 data; // gridSize, lineThick, camMult, gridRatio
    glm::ivec4 sideData;
} s_wallUbo_l;

class DiagonalLiner : public WallBase
{
public:
    DiagonalLiner(ThinDrawer* thinDrawer) : WallBase(thinDrawer) { }

    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet();

    void testFill();
    void recreateUniforms(int size);
};