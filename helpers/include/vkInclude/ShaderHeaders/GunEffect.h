#pragma once

#include <ShaderBase.h>

typedef struct GunData
{
    glm::vec4 positions;
    glm::vec4 color;
    glm::vec4 portal;
    glm::vec4 data; // ySize, timeMult, none, none
} s_gunData;

class GunEffect : public ShaderBase
{
    friend class ThinDrawer;

private:
    GunEffect(ThinDrawer* thinDrawer) : ShaderBase(thinDrawer) { }

    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet();
    void prepareVertexData();
};