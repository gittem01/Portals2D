#pragma once

#include <WallBase.h>

typedef struct
{
    glm::vec4 posSize;
    glm::vec4 color;
    glm::vec4 data; // lightXSize, lightYSize, upDownSize, lightDepth
    glm::vec4 data2; // rotation, glass rotation, camMult, lightScatterRatio
} s_walLightlUbo;

class WallLight : public WallBase
{
public:
    WallLight(ThinDrawer* thinDrawer) : WallBase(thinDrawer) { }

    int alignedSize;

    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet();

    void testFill();
    void recreateUniforms(int size);
};