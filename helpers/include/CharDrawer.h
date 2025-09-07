#pragma once

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <mutex>

#include <stb_truetype.h>
#include <FontShader.h>

typedef struct CharRenderData
{
	glm::vec4 size_leftBotMargin;
	s_texture* tex;
	int codePoint;
	int targetHeight;
} s_charRenderData;

typedef struct LineRenderData
{
	std::vector<int> charDatas;
	glm::vec4 posScale; // startX, startY, scale, unused
	glm::vec4 color;
} s_lineRenderData;

typedef struct fontInfoData
{
	stbtt_fontinfo info;
	std::vector<unsigned char> data;
} fontInfoData;

class CharDrawer
{
public:
	static int selectedLanguageIndex;
	
	class ThinDrawer* thinDrawer;
	FontShader* fontShader;
	std::vector<fontInfoData> fontInfos;
	int creationHeight;
	std::string languageName;
	std::vector<std::string> allLanguages;
	bool clear = true;

	std::vector<s_lineRenderData> renderDatas;
	std::vector<s_charRenderData> allCharDatas;
	std::map<std::pair<int, int>, int> codepointMap;
	std::mutex loadMutex;

	CharDrawer(ThinDrawer* thinDrawer);

	std::vector<int> getRenderData(const std::string& text, int targetHeight, float& x,
		s_texture* prevTex=nullptr, int prevCodepoint=0);

	float calcSize(std::vector<int> rd);
	static char calculateCharBytes(char ch);
	static void draw(VkCommandBuffer cmdBuff, void* ptr);

	void recreate();
	bool loadFontFile(const std::string& filename, std::vector<unsigned char>& fontData, stbtt_fontinfo& fontInfo);
};