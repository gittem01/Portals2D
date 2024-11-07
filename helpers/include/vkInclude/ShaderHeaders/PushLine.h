#pragma once

#include <ShaderBase.h>

class PushLine : public ShaderBase
{
public:
    PushLine(ThinDrawer* thinDrawer) : ShaderBase(thinDrawer) { }

    void prepareVertexData();
    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet();
};