#pragma once

#include <ShaderBase.h>

typedef struct TextData
{
    glm::vec4 data;
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