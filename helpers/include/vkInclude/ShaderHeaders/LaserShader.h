#pragma once

#include <ShaderBase.h>

typedef struct
{
    glm::vec4 points;
    glm::vec4 colour;
    glm::vec4 data; // thickness, none, none, none
} s_laserUbo;

class LaserShader : public ShaderBase
{
public:

    int numLasers = 0;

    LaserShader(ThinDrawer* thinDrawer) : ShaderBase(thinDrawer) { }

    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet();
    void prepareVertexData();
};