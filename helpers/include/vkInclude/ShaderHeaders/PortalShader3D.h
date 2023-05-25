#pragma once

#include <ShaderBase.h>
#include <glm/glm.hpp>

#define UNIFORM_ALLOC_COUNT 256

class PortalShader3D : public ShaderBase
{
    friend class ThinDrawer;
    friend class TestPlayer;

private:
    int alignedSize;
    int lastAllocationPos;
    uint32_t availableOffsets[UNIFORM_ALLOC_COUNT];

    PortalShader3D(ThinDrawer* thinDrawer) : ShaderBase(thinDrawer) { }

    static glm::vec2 getEllipsePosOnAngle(float a, float b, float angle);

    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet();
    void prepareVertexData();
};