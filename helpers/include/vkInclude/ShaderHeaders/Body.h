#pragma once

#include <ShaderBase.h>

#define UNIFORM_ALLOC_COUNT_BODY 10000
#define UNIFORM_ALLOC_COUNT_FIXTURE UNIFORM_ALLOC_COUNT_BODY * 4

class Body : public ShaderBase
{
    friend class ThinDrawer;

private:
    Body(ThinDrawer* thinDrawer) : ShaderBase(thinDrawer) { }

    int lastAllocationPos_body;
    int alignedSize_body;
    uint32_t availableOffsets_body[UNIFORM_ALLOC_COUNT_BODY];

    int lastAllocationPos_fixture;
    int alignedSize_fixture;
    uint32_t availableOffsets_fixture[UNIFORM_ALLOC_COUNT_FIXTURE];

    void prepareVertexData();
    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet();
};