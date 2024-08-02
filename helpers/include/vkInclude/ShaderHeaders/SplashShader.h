#pragma once

#include <ShaderBase.h>

typedef struct
{
    glm::vec4 posSpeed;
    glm::vec4 data; // radius, summonTime, angleMult, angleOffset
    glm::vec4 data2; // pow, powMult, speedMult, outRMult
    glm::vec4 data3; // addMult, unused, unused, unused
    glm::vec4 color;
} splashData;

class SplashShader : public ShaderBase
{
public:
    SplashShader(ThinDrawer* thinDrawer) : ShaderBase(thinDrawer) { }

    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet();
    void prepareVertexData();
};