#pragma once

#include <ShaderBase.h>

// per wall line
typedef struct
{
    glm::vec4 positions;
    glm::ivec4 data; // size, gelDataOffset, prevOffset, nextOffset
} s_wallGelData;

// in wall line
typedef struct
{
    glm::vec4 data; // colorIndex, gelHeight, time, force
} s_wallPerGelData;

class WallGel : public ShaderBase
{
public:
    int numWallLines = 0;
    int numWallGels = 0;

    WallGel(ThinDrawer* thinDrawer) : ShaderBase(thinDrawer) { }

    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet();
    void prepareVertexData();
};