#pragma once

#include <ShaderBase.h>
#include <glm/glm.hpp>

#define UNIFORM_ALLOC_COUNT 256

typedef struct s_portalPsh
{
    glm::vec4 posSize; // pos.x, pos.y, size.x, size.y
    glm::vec4 color;
    glm::vec4 data0; // dirAngle, pixeler, midThick, xDropSize
} s_portalPsh;

class PortalShader : public ShaderBase
{
    friend class ThinDrawer;
    friend class TestPlayer;

private:
    PortalShader(ThinDrawer* thinDrawer) : ShaderBase(thinDrawer) { }

    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet();
    void prepareVertexData();
};