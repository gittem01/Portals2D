#pragma once

#include <ShaderBase.h>

#define UNIFORM_ALLOC_COUNT_PLAYER 256
#define UNIFORM_ALLOC_COUNT_PLAYER_2 8


typedef struct legData
{
    float legAngle;
    float legThick;
    float circR;
    float legLen;
    float legOuterCircAng;
    float legCircleSize;
} legData;

typedef struct renderPlayerUbo
{
    glm::vec4 edgeColour;
    glm::vec4 bodyColour1;
    glm::vec4 bodyColour2;
    glm::vec2 size;
    glm::vec2 headCircles;
    float topMargin;
    float headRatio;
    float bodyRatio;
    float legRatio;
    float margin1;
    float margin2;
    float lineSize;
    float bodyCircles;
    legData ld;
} s_renderPlayerUbo;

typedef struct dRenderPlayerUbo
{
    glm::vec4 posRotD; // posx, posy, rotation, unused
    glm::vec4 portals[4];
    int numPortals;
} s_dRenderPlayerUbo;


class PlayerShader : public ShaderBase
{
    friend class ThinDrawer;
    friend class TestPlayer;

private:
    PlayerShader(ThinDrawer* thinDrawer) : ShaderBase(thinDrawer) { }

    int alignedSize;
    int alignedSize2;

    void prepareVertexData();
    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet() override;
};