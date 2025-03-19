#pragma once

#include <ShaderBase.h>

#define MAX_PLATFORMS 32

typedef struct
{
    glm::vec4 posSize;
    glm::vec4 inColour;
    glm::vec4 outColour;
    glm::vec4 data; // squarePattern, outerThick, outerSmooth, innerSmooth
} s_platformRectUbo;

class PlatformRectangle : public ShaderBase
{
public:

    int numPlatforms = 0;

    PlatformRectangle(ThinDrawer* thinDrawer) : ShaderBase(thinDrawer) { }

    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet();
    void prepareVertexData();
};