#pragma once

#include <ShaderBase.h>

class DebugPolygon : public ShaderBase
{
    friend class ThinDrawer;

private:
    DebugPolygon(ThinDrawer* thinDrawer) : ShaderBase(thinDrawer) { }

    void prepareVertexData();
    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet();
};