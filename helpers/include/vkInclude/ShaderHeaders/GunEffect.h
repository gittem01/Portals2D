#pragma once

#include <ShaderBase.h>

class GunEffect : public ShaderBase
{
    friend class ThinDrawer;

private:
    GunEffect(ThinDrawer* thinDrawer) : ShaderBase(thinDrawer) { }

    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet();
    void prepareVertexData();
};