#pragma once

#include <ShaderBase.h>
#include <PortalShader.h>
#include <glm/glm.hpp>

class EndlessGrid : public ShaderBase
{
    friend class ThinDrawer;
    friend class TestPlayer;

private:
    EndlessGrid(ThinDrawer* thinDrawer) : ShaderBase(thinDrawer) { }

    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet();
    void prepareVertexData();
};