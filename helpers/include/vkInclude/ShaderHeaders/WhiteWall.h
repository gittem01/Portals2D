#pragma once

#include <ShaderBase.h>

class WhiteWall : public ShaderBase
{
    friend class ThinDrawer;

private:
    WhiteWall(ThinDrawer* thinDrawer) : ShaderBase(thinDrawer) { }

    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet();
    void prepareVertexData();
};