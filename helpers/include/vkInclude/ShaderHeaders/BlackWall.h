#pragma once

#include <WallBase.h>

typedef struct
{
    glm::vec4 posSize;
    glm::vec4 colour;
    glm::vec4 data; // isVertical, lineSize, camMult, none
} s_wallUbo_b;

class BlackWall : public WallBase
{
public:
    BlackWall(ThinDrawer* thinDrawer) : WallBase(thinDrawer) { }

    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet();

    void testFill();
    void recreateUniforms(int size);
};