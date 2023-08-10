#pragma once

#include <ShaderBase.h>

class WallBase : public ShaderBase
{
public:
    static const int gridWidth = 1000;
    static const int gridHeight = 1000;
    static const int minX = -gridWidth / 2;
    static const int maxX = +gridWidth / 2 - 1;
    static const int minY = -gridHeight / 2;
    static const int maxY = +gridHeight / 2 - 1;
    static const int gridDivider = 50;
    static const int gridCount = gridWidth / gridDivider;
    static const float layerMult;

    std::vector<std::vector<std::vector<s_shaderGridData>>> renderGridUp;

    WallBase(ThinDrawer* thinDrawer) : ShaderBase(thinDrawer) { }

    void prepareVertexData();

    virtual void recreateUniforms(int size) = 0;
    virtual void testFill() = 0;

    s_gridPositionShader getGridSquare(float x, float y);
    s_gridPositionShader getGridSquare(int x, int y);
    void render(VkCommandBuffer cmdBuff, int layer, float camX, float camY);
};