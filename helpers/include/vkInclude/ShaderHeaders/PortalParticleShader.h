#pragma once

#include <ShaderBase.h>
#include <PortalShader.h>
#include <glm/glm.hpp>

typedef struct s_portalParticleUbo
{
    glm::vec4 portalPoints;
    glm::vec4 color;
    glm::vec4 data0; // dist, speed, time, upSize
    glm::vec4 data1; // sizex, sizey, randomization, instanceCount
} s_portalParticleUbo;

class PortalParticleShader : public ShaderBase
{
    friend class ThinDrawer;
    friend class TestPlayer;

private:
    int alignedSize;
    int lastAllocationPos;
    uint32_t availableOffsets[UNIFORM_ALLOC_COUNT];

    PortalParticleShader(ThinDrawer* thinDrawer) : ShaderBase(thinDrawer) { }

    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet();
    void prepareVertexData();
};