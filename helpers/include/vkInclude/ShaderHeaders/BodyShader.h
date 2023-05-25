#pragma once

#include <ShaderBase.h>

#define UNIFORM_ALLOC_COUNT_BODY 2500
#define UNIFORM_ALLOC_COUNT_FIXTURE UNIFORM_ALLOC_COUNT_BODY * 4

class BodyShader : public ShaderBase
{
	friend class ThinDrawer;

private:
    BodyShader(ThinDrawer* thinDrawer) : ShaderBase(thinDrawer) { }

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
    void setupDescriptorSet() override;
};