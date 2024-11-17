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
	std::vector<unsigned char> fontData;
	stbtt_fontinfo fontInfo;
	int selectedLanguageIndex;
	int creationHeight;
	std::string languageName;

	std::vector<s_lineRenderData> renderDatas;
	std::map<std::pair<int, int>, s_charRenderData> codepointMap;
	std::map<int, std::string> polarizationString;

	CharDrawer(ThinDrawer* thinDrawer);

	std::vector<s_charRenderData> getRenderData(const std::string& text, int targetHeight, float& x);

	static float calcSize(std::vector<s_charRenderData> rd);
	static void draw(VkCommandBuffer cmdBuff, void* ptr);

	bool loadFontFile(const std::string& filename, std::vector<unsigned char>& fontData, stbtt_fontinfo& fontInfo);
};