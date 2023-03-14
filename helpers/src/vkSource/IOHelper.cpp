#include <IOHelper.h>

#include <tiny_gltf.h>
#include <glm/glm.hpp>
#include <ThinDrawer.h>
#include <vkInit.h>

std::string IOHelper::assetsFolder;
std::map<std::string, s_meshData*> IOHelper::assetMap;
std::map<std::string, struct s_texture*> IOHelper::textureMap;
ThinDrawer* IOHelper::thinDrawer;
VkDescriptorSetLayout IOHelper::textureSetLayout;

void IOHelper::getAssetsFolder()
{
	if (searchDepth < 1)
	{
		assert("number of trials is smaller than 1");
	}

	char pathString[3 * searchDepth + 1 + sizeofAssetsFolderName];

	for (int currentTry = 0; currentTry < searchDepth; currentTry++)
	{
		int pathStringSize = currentTry * 3 + sizeofAssetsFolderName;
		pathString[pathStringSize] = '\0';

		for (int i = 0; i < currentTry; i++)
		{
			pathString[i * 3 + 0] = '.';
			pathString[i * 3 + 1] = '.';
			pathString[i * 3 + 2] = '/';
		}

		for (int i = 0; i < sizeofAssetsFolderName - 1; i++)
		{
			pathString[currentTry * 3 + i] = assetsFolderName[i];
		}
		pathString[currentTry * 3 + sizeofAssetsFolderName - 1] = '/';

		if (std::filesystem::exists(pathString))
		{
			assetsFolder = std::string(pathString, pathStringSize);
			return;
		}
	}

	assetsFolder = "assets/";

	printf("Could not find assets folder. Assigning it as '/assets'\n");
}

void IOHelper::loadGlTF(std::string fileName)
{
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string warn;
    std::string err;

    bool out = loader.LoadBinaryFromFile(&model, &err, &warn, IOHelper::assetsFolder + "gltf/" + fileName + ".glb");
    if (!out)
    {
        printf("Failed to parse glTF\n");
    }
    if (!warn.empty())
    {
        printf("Warn: %s\n", warn.c_str());
    }
    if (!err.empty())
    {
        printf("Err: %s\n", err.c_str());
    }

    for (tinygltf::Mesh mesh : model.meshes)
    {
        std::string fullString = fileName + "." + mesh.name;
        s_meshData* meshData = new s_meshData;
        assetMap[fullString] = meshData;
        for (int i = 0; i < 2; i++)
        {
            meshData->vertexBuffers[i] = (s_buffers*)malloc(sizeof(s_buffers));
            meshData->indexBuffers[i] = (s_buffers*)malloc(sizeof(s_buffers));
            meshData->minMaxValues[i].resize(3);
        }
        
        std::vector<s_averageVertex> vertexData;
        std::vector<uint8_t> indices;

        std::vector<s_averageVertex> vertexData_m;
        std::vector<uint8_t> indices_m;

        int positionsIndex = mesh.primitives[0].attributes["POSITION"];
        int normalsIndex = mesh.primitives[0].attributes["NORMAL"];
        int texCoordIndex = mesh.primitives[0].attributes["TEXCOORD_0"];
        int indicesIndex = mesh.primitives[0].indices;

        assert(model.accessors[positionsIndex].count == model.accessors[normalsIndex].count);

        int numVertices = model.accessors[positionsIndex].count;
        int numIndices = model.accessors[indicesIndex].count;

        meshData->indexBuffers[0]->count = numIndices;
        meshData->indexBuffers[1]->count = numIndices;

        int positionsIndex_bv = model.accessors[positionsIndex].bufferView;
        int normalsIndex_bv = model.accessors[normalsIndex].bufferView;
        int texCoordsIndex_bv = model.accessors[texCoordIndex].bufferView;
        int indicesIndex_bv = model.accessors[indicesIndex].bufferView;

        meshData->indexByteSize = model.bufferViews[indicesIndex_bv].byteLength / numIndices;

        for (int i = 0; i < 3; i++)
        {
            meshData->minMaxValues[0][i] = model.accessors[positionsIndex].minValues[i];
            meshData->minMaxValues[1][i] = model.accessors[positionsIndex].maxValues[i];
        }

        vertexData.resize(numVertices);
        indices.resize(meshData->indexByteSize* numIndices);

        vertexData_m.resize(vertexData.size());
        indices_m.resize(indices.size());

        memcpy(indices.data(), model.buffers[model.bufferViews[indicesIndex_bv].buffer].data.data() + model.bufferViews[indicesIndex_bv].byteOffset, indices.size());

        for (int i = 0; i < numIndices; i++)
        {
            int startByte = i * meshData->indexByteSize;
            int endByte = (numIndices - i - 1) * meshData->indexByteSize;
            memcpy(indices_m.data() + startByte, indices.data() + endByte, meshData->indexByteSize);
        }

        for (int i = 0; i < numVertices; i++)
        {
            s_averageVertex* currentVertexData = vertexData.data() + i;
            s_averageVertex* currentVertexData_m = vertexData_m.data() + i;

            memcpy(((unsigned char*)currentVertexData) + offsetof(s_averageVertex, pos),
                model.buffers[model.bufferViews[positionsIndex_bv].buffer].data.data() +
                model.bufferViews[positionsIndex_bv].byteOffset + i * sizeof(glm::vec3), sizeof(glm::vec3));

            memcpy(((unsigned char*)currentVertexData) + offsetof(s_averageVertex, normal),
                model.buffers[model.bufferViews[normalsIndex_bv].buffer].data.data() +
                model.bufferViews[normalsIndex_bv].byteOffset + i * sizeof(glm::vec3), sizeof(glm::vec3));

            memcpy(((unsigned char*)currentVertexData) + offsetof(s_averageVertex, tc),
                model.buffers[model.bufferViews[texCoordsIndex_bv].buffer].data.data() +
                model.bufferViews[texCoordsIndex_bv].byteOffset + i * sizeof(glm::vec2), sizeof(glm::vec2));

            memcpy(((unsigned char*)currentVertexData_m) + offsetof(s_averageVertex, tc),
                model.buffers[model.bufferViews[texCoordsIndex_bv].buffer].data.data() +
                model.bufferViews[texCoordsIndex_bv].byteOffset + i * sizeof(glm::vec2), sizeof(glm::vec2));

            currentVertexData_m->pos = glm::vec3(-currentVertexData->pos.x, currentVertexData->pos.y, currentVertexData->pos.z);
            currentVertexData_m->normal = glm::vec3(-currentVertexData->normal.x, currentVertexData->normal.y, currentVertexData->normal.z);
        }

        thinDrawer->bufferStage(vertexData.data(), vertexData.size() * sizeof(s_averageVertex),
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, meshData->vertexBuffers[0]);

        thinDrawer->bufferStage(vertexData_m.data(), vertexData_m.size() * sizeof(s_averageVertex),
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, meshData->vertexBuffers[1]);


        thinDrawer->bufferStage(indices.data(), indices.size(),
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, meshData->indexBuffers[0]);

        thinDrawer->bufferStage(indices_m.data(), indices_m.size(),
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, meshData->indexBuffers[1]);
    }
}

void IOHelper::loadTexture(std::string fileName)
{
    s_texture* tex = (s_texture*)malloc(sizeof(s_texture));
    thinDrawer->loadTexture(("textures/" + fileName).data(), tex);

    VkDescriptorSetLayoutBinding textureBind = vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 0);

    VkDescriptorSetLayoutBinding texBinding[] = { textureBind };

    VkDescriptorSetLayoutCreateInfo setInfo = vkinit::descriptorSetLayoutCreateInfo(texBinding, std::size(texBinding));
    CHECK_RESULT_VK(vkCreateDescriptorSetLayout(thinDrawer->logicalDevice, &setInfo, VK_NULL_HANDLE, &textureSetLayout));

    VkDescriptorSetAllocateInfo allocInfo = vkinit::descriptorSetAllocateInfo(thinDrawer->descriptorPool, &textureSetLayout, 1);
    vkAllocateDescriptorSets(thinDrawer->logicalDevice, &allocInfo, &tex->set);

    VkDescriptorImageInfo imageBufferInfo;
    imageBufferInfo.imageView = tex->view;
    imageBufferInfo.sampler = tex->sampler;
    imageBufferInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet textureWrite = { };
    textureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    textureWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    textureWrite.dstSet = tex->set;
    textureWrite.descriptorCount = 1;
    textureWrite.pImageInfo = &imageBufferInfo;
    textureWrite.dstBinding = 0;

    vkUpdateDescriptorSets(thinDrawer->logicalDevice, 1, &textureWrite, 0, VK_NULL_HANDLE);

    textureMap[fileName] = tex;
}