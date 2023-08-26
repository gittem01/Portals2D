#pragma once

#include <ShaderBase.h>

typedef struct
{
    // ax^2 + bx + c
    glm::vec4 equation; // a, b, c, thickness
    glm::vec4 data; // dotRatio, startX, xLen, unused
} s_polynomUbo;

typedef struct
{
    s_polynomUbo data;
    bool shouldRender;
} s_polynomRenderData;

class PolynomialShader : public ShaderBase
{
public:
    std::vector<s_polynomRenderData> datas;
    std::vector<int> freeIndices;

    PolynomialShader(ThinDrawer* thinDrawer) : ShaderBase(thinDrawer) { }

    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet();
    void prepareVertexData();

    void render(VkCommandBuffer cmdBuff, class Camera* cam);
};