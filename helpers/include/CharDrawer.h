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

typedef struct LineRenderData
{
	std::vector<s_charRenderData> charDatas;
	glm::vec4 posScale; // startX, startY, scale, unused
	glm::vec4 color;
} s_lineRenderData;

class CharDrawer
{
public:
	class ThinDrawer* thinDrawer;
	FontShader* fontShader;
	stbtt_fontinfo fontInfo;

	std::vector<s_lineRenderData> renderDatas;
	std::map<std::pair<int, int>, s_charRenderData> codepointMap;

	CharDrawer(ThinDrawer* thinDrawer, const char* fontFile);

	std::vector<s_charRenderData> getRenderData(const std::string& text, float targetHeight);

	static void draw(VkCommandBuffer cmdBuff, void* ptr);

	bool loadFontFile(const std::string& filename, std::vector<unsigned char>& fontData, stbtt_fontinfo& fontInfo);
};