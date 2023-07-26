#pragma once

#include <ShaderBase.h>

typedef struct
{
    glm::vec4 posSize;
    glm::vec4 bgColour;
    glm::vec4 dotColour;
    glm::vec4 edgeColour;
    glm::vec4 data; // dotSize, sideMargin, sideMult, smoothRatio
    glm::vec4 data2; // unused, unused, camMult, unused
} s_wallUbo_w;

class WhiteWall : public ShaderBase
{
public:
    WhiteWall(ThinDrawer* thinDrawer) : ShaderBase(thinDrawer) { }

    int wallCount;

    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet();
    void prepareVertexData();

    void testFill();
};