#pragma once

#include <ShaderBase.h>

class LaserPshShader : public ShaderBase
{
public:

    LaserPshShader(ThinDrawer* thinDrawer) : ShaderBase(thinDrawer) { }

    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet();
    void prepareVertexData();
};