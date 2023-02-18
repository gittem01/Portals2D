#pragma once

#include <ShaderBase.h>

class VulkanTriangle : public ShaderBase
{
	friend class ThinDrawer;

private:
    VulkanTriangle(ThinDrawer* thinDrawer) : ShaderBase(thinDrawer) { }

    void prepareVertexData();
    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet() override;
};