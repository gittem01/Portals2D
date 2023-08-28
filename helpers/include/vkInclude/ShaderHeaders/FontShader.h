#pragma once

#include <ShaderBase.h>

typedef struct TextData
{
    glm::vec4 data;
    glm::vec4 color;
} s_textData;

class FontShader : public ShaderBase
{
public:
    FontShader(ThinDrawer* thinDrawer) : ShaderBase(thinDrawer) { }

    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet();
    void prepareVertexData();
};