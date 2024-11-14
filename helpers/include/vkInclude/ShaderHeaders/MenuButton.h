#pragma once

#include <ShaderBase.h>

typedef struct
{
    glm::vec4 posSize;
    glm::vec4 colour;
    glm::vec4 data; // extraFill, smoothFill, unused, unused
} s_menuButton;

class MenuButton : public ShaderBase
{
public:
    MenuButton(ThinDrawer* thinDrawer) : ShaderBase(thinDrawer) { }

    void prepareVertexData();
    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet();
};