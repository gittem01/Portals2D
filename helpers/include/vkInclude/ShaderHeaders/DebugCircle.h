#pragma once

#include <ShaderBase.h>

class DebugCircle : public ShaderBase
{
    friend class ThinDrawer;

private:
    DebugCircle(ThinDrawer* thinDrawer) : ShaderBase(thinDrawer) { }

    void prepareVertexData();
    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet();
};