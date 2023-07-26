#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cassert>

#include <map>
#include <vector>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

typedef struct MeshData
{
	struct s_buffers* vertexBuffers[2];
	struct s_buffers* indexBuffers[2];
	std::vector<double> minMaxValues[2];
	uint8_t indexByteSize;
} s_meshData;

class ThinDrawer;

class IOHelper
{
public:
	static ThinDrawer* thinDrawer;

	static const int searchDepth = 4;
	static inline char assetsFolderName[] = "assets";
	static const int sizeofAssetsFolderName = sizeof(assetsFolderName) / sizeof(char);
	// ex : ../../%assetsFolderName%/
	static std::string assetsFolder;

	// only binding 0
	static VkDescriptorSetLayout textureSetLayout;

	static std::map<std::string, s_meshData*> assetMap;
	static std::map<std::string, struct s_texture*> textureMap;

	static void getAssetsFolder();
	// file name without .glb
	static void loadGlTF(std::string fileName);
	static void loadTexture(std::string fileName,
		VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT, VkFilter filter = VK_FILTER_LINEAR);
	static void updateTextureSet(s_texture* tex, bool preAllocated=false);
};