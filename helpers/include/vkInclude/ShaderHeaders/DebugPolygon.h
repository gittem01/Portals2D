#pragma once

#include <ShaderBase.h>

typedef struct
{
    glm::vec4 polyPoints[4];
    int numPolyPoints;
} s_uboPolyData;

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