#pragma once

#include <WallBase.h>

typedef struct
{
    glm::vec4 posSize;
    glm::vec4 bgColour;
    glm::vec4 dotColour;
    glm::vec4 edgeColour;
    glm::vec4 data; // dotSize, sideMargin, sideMult, smoothRatio
    glm::vec4 data2; // unused, unused, camMult, unused
} s_wallUbo_w;

class WhiteWall : public WallBase
{
public:
    WhiteWall(ThinDrawer* thinDrawer) : WallBase(thinDrawer) { }

    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet();

    ///////////
    void recreateUniforms(int size);
    void testFill();
};