#pragma once

#include <ShaderBase.h>

typedef struct MB
{
    glm::vec4 posSize;
    glm::vec4 colour;
    glm::vec4 data = glm::vec4(0.0f, 0.1f, 1.0f, 0.0f); // extraFill, smoothFill, bgNoiseVal, unused

    bool hoverCheck(glm::vec2 pos)
    {
        glm::vec2 size = (glm::vec2(posSize.z, posSize.w) + data.y) * 0.5f;

        float right = posSize.x + size.x;
        float top = posSize.y + size.y;
        float left = posSize.x - size.x;
        float bottom = posSize.y - size.y;
        if (pos.x >= left && pos.x <= right && pos.y >= bottom && pos.y <= top)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
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