#pragma once

#include <ShaderBase.h>
#include <glm/glm.hpp>

typedef struct
{
    glm::vec4 posSize;
    glm::vec4 color;
    glm::vec4 data; // lightXSize, lightYSize, upDownSize, lightDepth
    glm::vec4 data2; // rotation, glass rotation, camMult, lightScatterRatio
} s_walLightlUbo;

class WallLight : public ShaderBase
{
    friend class ThinDrawer;

private:
    WallLight(ThinDrawer* thinDrawer) : ShaderBase(thinDrawer) { }

    int alignedSize;
    int lightCount;

    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet();
    void prepareVertexData();
};