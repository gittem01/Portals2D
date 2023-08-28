#pragma once

#include <ShaderBase.h>

typedef struct TextData
{
    glm::vec4 posSize;
    glm::ivec4 data;
} s_optionData;

class OptionShader : public ShaderBase
{
public:
    OptionShader(ThinDrawer* thinDrawer) : ShaderBase(thinDrawer) { }

    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet();
    void prepareVertexData();
};