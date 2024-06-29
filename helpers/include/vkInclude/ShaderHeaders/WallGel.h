#pragma once

#include <ShaderBase.h>

typedef struct WalGellData
{
    glm::vec4 positions;
    glm::vec4 data;
} s_wallGelData;

class WallGel : public ShaderBase
{
public:
    WallGel(ThinDrawer* thinDrawer) : ShaderBase(thinDrawer) { }

    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet();
    void prepareVertexData();
};