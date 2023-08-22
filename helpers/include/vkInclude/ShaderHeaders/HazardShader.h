#pragma once

#include <ShaderBase.h>

typedef struct
{
    glm::vec4 pointts;
    glm::vec4 colour;
    glm::vec4 colour2;
    glm::vec4 data; // thickness, timeOffset, status, statusChangeTime
} s_hazardUbo;

class HazardShader : public ShaderBase
{
public:

    int numHazards = 0;

    HazardShader(ThinDrawer* thinDrawer) : ShaderBase(thinDrawer) { }

    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet();
    void prepareVertexData();
};