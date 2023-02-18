#pragma once

#include <ShaderBase.h>

class DebugLine : public ShaderBase
{
    friend class ThinDrawer;

private:
    DebugLine(ThinDrawer* thinDrawer) : ShaderBase(thinDrawer) { }

    void prepareVertexData();
    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet();
};