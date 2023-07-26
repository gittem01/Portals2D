#pragma once

#include <ShaderBase.h>

typedef struct
{
    glm::vec4 posSize;
    glm::vec4 colour;
    glm::vec4 data; // isVertical, lineSize, camMult, none
} s_wallUbo_b;

class BlackWall : public ShaderBase
{
public:
    BlackWall(ThinDrawer* thinDrawer) : ShaderBase(thinDrawer) { }

    int wallCount;

    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet();
    void prepareVertexData();

    void testFill();
};