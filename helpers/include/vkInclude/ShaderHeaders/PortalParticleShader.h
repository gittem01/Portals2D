#pragma once

#include <ShaderBase.h>
#include <PortalShader.h>
#include <glm/glm.hpp>

typedef struct s_portalParticlePsh
{
    glm::vec4 portalPoints;
    glm::vec4 color;
    glm::vec4 data0; // dist, speed, time, upSize
    glm::vec4 data1; // sizex, sizey, randomization, instanceCount
} s_portalParticlePsh;

class PortalParticleShader : public ShaderBase
{
    friend class ThinDrawer;
    friend class TestPlayer;

private:
    PortalParticleShader(ThinDrawer* thinDrawer) : ShaderBase(thinDrawer) { }

    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet();
    void prepareVertexData();
};