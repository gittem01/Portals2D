#pragma once

#include <ShaderBase.h>

#define UNIFORM_ALLOC_COUNT_BODY 2500
#define UNIFORM_ALLOC_COUNT_FIXTURE UNIFORM_ALLOC_COUNT_BODY * 4

typedef struct RenderBodyPush
{
    glm::vec4 posRot; // posx, posy, rot, mirrorStatus(<0 for mirrored)
    glm::vec4 movScale; // xMov, yMov, xScale, yScale (movement and scaling before rotation)
    glm::vec4 portals[4];
    int numPortals;
} s_renderBodyPsh;

class BodyShader : public ShaderBase
{
	friend class ThinDrawer;

private:
    BodyShader(ThinDrawer* thinDrawer) : ShaderBase(thinDrawer) { }

    void prepareVertexData();
    void prepareUniforms();
    void setupDescriptorSetLayout();
    void preparePipeline();
    void setupDescriptorSet() override;
};