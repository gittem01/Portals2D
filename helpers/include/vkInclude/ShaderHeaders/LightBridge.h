#pragma once

#include <ShaderBase.h>

typedef struct
{
    glm::vec4 points;
    glm::vec4 data; // thickness, startX, none, none
    glm::vec4 portal;
} s_bridgeUbo;

class LightBridge : public ShaderBase
{
public:

    int numbridges = 0;

    LightBridge(ThinDrawer* thinDrawer) : ShaderBase(thinDrawer) { }

    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet();
    void prepareVertexData();
};