#pragma once

#include <ShaderBase.h>
#include <glm/glm.hpp>

#define UNIFORM_ALLOC_COUNT 256

typedef struct s_portalUbo3D
{
    glm::mat4 modelMatrix;
    glm::vec4 data0; // a, b, abRatio, eThick
    glm::vec4 data1; // lineLength, maxAngleInc, minAngleInc, noiseScale
    glm::vec4 data2; // sizex, sizey, iTime, 0
    glm::vec4 data3; // funcMult0, funcMult1, noiseMult, 0
    glm::vec4 portalColour;
    glm::vec4 edgeColour;
} s_portalUbo3D;

class PortalShader3D : public ShaderBase
{
    friend class ThinDrawer;
    friend class TestPlayer;

private:
    int alignedSize;
    int lastAllocationPos;
    uint32_t availableOffsets[UNIFORM_ALLOC_COUNT];

    PortalShader3D(ThinDrawer* thinDrawer) : ShaderBase(thinDrawer) { }

    static glm::vec2 getEllipsePosOnAngle(float a, float b, float angle);

    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet();
    void prepareVertexData();
};