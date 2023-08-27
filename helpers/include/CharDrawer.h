#pragma once

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>

#include <stb_truetype.h>
#include <FontShader.h>

typedef struct CharRenderData
{
	glm::vec4 size_leftBotMargin;
	s_texture* tex;
} s_charRenderData;

class CharDrawer
{
public:
	class ThinDrawer* thinDrawer;
	FontShader* fontShader;

	std::vector<std::vector<s_charRenderData>> renderDatas;
	std::map<int, s_charRenderData> codepointMap;

	CharDrawer(ThinDrawer* thinDrawer, const char* fontFile);

	static void draw(VkCommandBuffer cmdBuff, void* ptr);

	bool loadFontFile(const std::string& filename, std::vector<unsigned char>& fontData, stbtt_fontinfo& fontInfo);
};