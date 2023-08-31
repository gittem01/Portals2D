#pragma once

#include <ShaderBase.h>

typedef struct
{
    glm::vec4 points;
    glm::vec4 colour;
    glm::vec4 data; // thickness, none, none, none
} s_lineZUbo;

class LineZ : public ShaderBase
{
public:

    int numZs = 0;

    LineZ(ThinDrawer* thinDrawer) : ShaderBase(thinDrawer) { }

    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet();
    void prepareVertexData();
};