#pragma once

#include <ShaderBase.h>

#define MAX_FUNNELS 32

typedef struct
{
    glm::vec4 pointts;
    glm::vec4 colour;
    glm::vec4 colour2;
    glm::vec4 data; // thickness, offsetX, startX, colourInd
    glm::vec4 data2; // splitX, endSmoothPosX, totalX, splitTime
} s_funnelUbo;

class FunnelShader : public ShaderBase
{
public:

    int numFunnels = 0;

    FunnelShader(ThinDrawer* thinDrawer) : ShaderBase(thinDrawer) { }

    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet();
    void prepareVertexData();
};