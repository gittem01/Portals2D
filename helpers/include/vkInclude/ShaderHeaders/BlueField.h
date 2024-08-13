#pragma once

#include <ShaderBase.h>

typedef struct
{
    glm::vec4 points;
    glm::vec4 colour;
    glm::vec4 colour2;
    glm::vec4 data; // thickness, timeOffset, status, statusChangeTime
    glm::vec4 shootData; // shootPos.x, shootPos.y, shootTime, unused
} s_blueFieldUbo;

class BlueField : public ShaderBase
{
public:

    int numFields = 0;

    BlueField(ThinDrawer* thinDrawer) : ShaderBase(thinDrawer) { }

    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet();
    void prepareVertexData();
};