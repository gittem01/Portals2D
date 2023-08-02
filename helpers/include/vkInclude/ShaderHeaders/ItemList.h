#pragma once

#include <ShaderBase.h>

typedef struct ItemData
{
    glm::vec4 posSize;
    int data[4];
} s_itemData;

class ItemList : public ShaderBase
{
public:
    ItemList(ThinDrawer* thinDrawer) : ShaderBase(thinDrawer) { }

    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet();
    void prepareVertexData();
};