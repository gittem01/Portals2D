#pragma once

#include <ShaderBase.h>

typedef struct
{
    glm::vec4 positions;
    glm::vec4 colour;
    glm::vec4 data; // unused, lineThickness, outerSmoothThicknessRatio, unused
} s_debugLineUbo;

class DebugLine : public ShaderBase
{
    friend class ThinDrawer;
    friend class BaseTestCase;

private:
    DebugLine(ThinDrawer* thinDrawer) : ShaderBase(thinDrawer) { }

    int numLines = 0;

    void prepareVertexData();
    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet();
};