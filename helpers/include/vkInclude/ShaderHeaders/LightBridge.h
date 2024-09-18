#pragma once

#include <ShaderBase.h>

typedef struct
{
    glm::vec4 points;
    glm::vec4 data; // thickness, totalSize, clrr, offset
} s_bridgeUbo;

class LightBridgeShader : public ShaderBase
{
public:

    int numbridges = 0;

    LightBridgeShader(ThinDrawer* thinDrawer) : ShaderBase(thinDrawer) { }

    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet();
    void prepareVertexData();
};