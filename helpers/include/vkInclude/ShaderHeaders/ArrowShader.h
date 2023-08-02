#pragma once

#include <ShaderBase.h>

typedef struct ArrowData
{
    glm::vec4 posSize;
    glm::vec4 color;
    glm::vec4 data; // h1x, h2x, vx, vy
    glm::vec4 data2; // rot, cr1, cr2, cr3
} s_arrowData;

class ArrowShader : public ShaderBase
{
public:
    ArrowShader(ThinDrawer* thinDrawer) : ShaderBase(thinDrawer) { }

    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet();
    void prepareVertexData();
};