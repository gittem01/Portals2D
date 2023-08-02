#pragma once

#include <ShaderBase.h>

typedef struct ManyTextureData
{
    glm::vec4 posSize;
    glm::vec4 data;
} s_manyTextureData;

class ManyTexture : public ShaderBase
{
public:

    int lastAvailablePos = 0;

    ManyTexture(ThinDrawer* thinDrawer) : ShaderBase(thinDrawer) { }

    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet();
    void prepareVertexData();
};