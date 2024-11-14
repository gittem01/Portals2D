#include <WallBase.h>
#include <ThinDrawer.h>

const float WallBase::layerMult = 0.8;

s_gridPositionShader WallBase::getGridSquare(float x, float y)
{
    return { (short)std::floor((x - minX) / gridDivider), (short)std::floor((y - minY) / gridDivider) };
}

s_gridPositionShader WallBase::getGridSquare(int x, int y)
{
    return { (short)((x - minX) / gridDivider), (short)((y - minY) / gridDivider) };
}

void WallBase::prepareVertexData()
{
    const int bufferSize = 1;
    buffers.resize(bufferSize);

    s_buffers* vertices = &buffers[0];

    std::vector<s_basicVertex> vertexBuffer =
    {
            { {-0.5f, +0.5f} },
            { {+0.5f, +0.5f} },
            { {+0.5f, -0.5f} },
            { {+0.5f, -0.5f} },
            { {-0.5f, -0.5f} },
            { {-0.5f, +0.5f} },
    };

    uint32_t vertexBufferSize = static_cast<uint32_t>(vertexBuffer.size()) * sizeof(s_basicVertex);
    vertices->count = vertexBuffer.size();

    thinDrawer->bufferStage(vertexBuffer.data(), vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, vertices);
}

void WallBase::render(VkCommandBuffer cmdBuff, int layer, float camX, float camY)
{
    if (renderGridUp.size() == 0) return;

    VkDeviceSize offsets = 0;
    const uint32_t camOffset = ShaderBase::alignedSharedSize * thinDrawer->lastSwapChainImageIndex;
    s_gridPositionShader p = getGridSquare(camX, camY);
    const int n = 1;

    vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    vkCmdBindVertexBuffers(cmdBuff, 0, 1, &buffers[0].buffer, &offsets);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

    vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipelineLayout, 1, 1, &descriptorSets[1], 0, VK_NULL_HANDLE);

    if (textureData.size() != 0)
    {
        vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout, 2, 1, &textureData[0]->set, 0, VK_NULL_HANDLE);
    }

    for (int i = std::max(p.y - n, 0); i <= std::min(p.y + n, gridCount - 1); i++)
    {
        for (int j = std::max(p.x - n, 0); j <= std::min(p.x + n, gridCount - 1); j++)
        {
            s_shaderGridData& data = renderGridUp[layer][i][j];

            if (data.numElements == 0) continue;

            vkCmdDraw(cmdBuff, buffers[0].count, data.numElements, 0, data.offsetStart);
        }
    }
}