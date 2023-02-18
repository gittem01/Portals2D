#include <ShaderBase.h>
#include <ThinDrawer.h>
#include <SwapChain.h>
#include <vkInit.h>

#define ALLOCATION_COUNT_ALL 2000
#define ALLOCATION_COUNT_POLY 2000

ShaderBase::ShaderBase(ThinDrawer* thinDrawer)
{
    this->thinDrawer = thinDrawer;
    this->logicalDevice = thinDrawer->logicalDevice;
}

void ShaderBase::createCommonStorageBuffers(ThinDrawer* thinDrawer)
{
    const int bufferSize = 2;

    int imCount = thinDrawer->swapChain->imageCount;

    commonStorageBuffers.resize(imCount);

    int alignedSizeFS = thinDrawer->pad_storage_buffer_size(sizeof(s_uboAll));
    int alignedSizePoly = thinDrawer->pad_storage_buffer_size(sizeof(s_uboPolyData));

    alignedSizes = { alignedSizeFS, alignedSizePoly };

    for (int i = 0; i < imCount; i++)
    {
        commonStorageBuffers[i].resize(bufferSize);

        for (int j = 0; j < bufferSize; j++)
        {
            commonStorageBuffers[i][j] = (s_uniformBuffer*)malloc(sizeof(s_uniformBuffer));
        }

        initialAllocationSizes.resize(bufferSize);
        avlAllocLastLoc.resize(bufferSize);
        freeIndicesList.resize(bufferSize);

        initialAllocationSizes[0] = ALLOCATION_COUNT_ALL;
        initialAllocationSizes[1] = ALLOCATION_COUNT_POLY;

        avlAllocLastLoc[0] = initialAllocationSizes[0] - 1;
        avlAllocLastLoc[1] = initialAllocationSizes[1] - 1;

        for (int j = 0; j < bufferSize; j++)
        {
            freeIndicesList[j] = (int*)malloc(sizeof(int) * initialAllocationSizes[j]);

            for (int k = 0; k < initialAllocationSizes[j]; k++)
            {
                freeIndicesList[j][initialAllocationSizes[j] - k - 1] = k;
            }

            thinDrawer->uniformHelper(alignedSizeFS * initialAllocationSizes[j],
                commonStorageBuffers[i][j], VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, initialAllocationSizes[j]);
        }
    }
}

void ShaderBase::setupDescriptorSet()
{
    int imCount = thinDrawer->swapChain->imageCount;
    descriptorSets.resize(2);
    descriptorSets[0].resize(imCount);

    for (int i = 0; i < imCount; i++)
    {
        VkDescriptorSetAllocateInfo allocInfo = vkinit::descriptorSetAllocateInfo(thinDrawer->descriptorPool, &descriptorSetLayouts[0], 1);
        CHECK_RESULT_VK(vkAllocateDescriptorSets(logicalDevice, &allocInfo, &descriptorSets[0][i]));

        VkWriteDescriptorSet writeDescriptorSet = { };

        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.dstSet = descriptorSets[0][i];
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

        writeDescriptorSet.pBufferInfo = &thinDrawer->sharedOrthoUniform[i]->descriptor;
        writeDescriptorSet.dstBinding = 0;
        vkUpdateDescriptorSets(logicalDevice, 1, &writeDescriptorSet, 0, VK_NULL_HANDLE);
        if (uniformBuffers.size() == 0) continue;

        for (int j = 0; j < uniformBuffers[i].size(); j++)
        {
            writeDescriptorSet.pBufferInfo = &uniformBuffers[i][j]->descriptor;
            writeDescriptorSet.dstBinding = j + 1;

            vkUpdateDescriptorSets(logicalDevice, 1, &writeDescriptorSet, 0, VK_NULL_HANDLE);
        }
    }
}

std::vector<std::vector<s_uniformBuffer*>> ShaderBase::commonStorageBuffers;
std::vector<int> ShaderBase::initialAllocationSizes;
std::vector<int> ShaderBase::avlAllocLastLoc;
std::vector<int*> ShaderBase::freeIndicesList;
std::vector<int64_t> ShaderBase::alignedSizes;