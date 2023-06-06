#pragma once

#include <ShaderBase.h>

typedef struct
{
    glm::mat4 transformMatrix;
    glm::mat4 rotationMatrix;
    glm::vec4 bgColour;
    glm::vec4 dotColour;
    glm::vec4 edgeColour;
    glm::vec4 leftUp;
    glm::vec4 data; // dotSize, sideMargin, sideMult, smoothRatio
    glm::vec4 data2; // sizeX, sizeY, unused, unused
} s_wallUbo_w;

class WhiteWall : public ShaderBase
{
    friend class ThinDrawer;

private:
    WhiteWall(ThinDrawer* thinDrawer) : ShaderBase(thinDrawer) { }

    int wallCount;

    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet();
    void prepareVertexData();
};