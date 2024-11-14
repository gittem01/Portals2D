#pragma once

#include <ShaderBase.h>
#include <PortalShader.h>
#include <glm/glm.hpp>

typedef struct s_dualParticleUbo
{
    glm::vec4 color0;
    glm::vec4 color1;
    glm::vec4 data0; // none, speed, time, midCircleRadius
    glm::vec4 data1; // sizex, sizey, randomization, none
} s_dualParticleUbo;

class DualParticleShader : public ShaderBase
{
public:
    DualParticleShader(ThinDrawer* thinDrawer) : ShaderBase(thinDrawer) { }

    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet();
    void prepareVertexData();
};