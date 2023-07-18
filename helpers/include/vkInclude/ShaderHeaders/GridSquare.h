#pragma once

#include <ShaderBase.h>
#include <PortalShader.h>
#include <glm/glm.hpp>

typedef struct GridSquareData
{
    int positionData[4]; // posx, posy, numInstances, data1
    glm::vec4 colour;
} s_gridSquareData;

class GridSquare : public ShaderBase
{
    friend class ThinDrawer;
    friend class TestPlayer;

private:
    GridSquare(ThinDrawer* thinDrawer) : ShaderBase(thinDrawer) { }

    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet();
    void prepareVertexData();
};