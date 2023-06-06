#pragma once

#include <ShaderBase.h>

typedef struct
{
    glm::vec4 positions;
    glm::vec4 colour;
    glm::vec4 data; // unused, lineThickness, outerSmoothThicknessRatio, unused
} s_pushConstantLine;

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