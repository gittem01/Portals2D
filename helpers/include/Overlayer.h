#pragma once

#include <vulkan/vulkan.h>
#include <Camera.h>
#include <Leveler.h>

class Overlayer
{
public:
	class Game* game;
	class ThinDrawer* thinDrawer;
	class WindowHandler* wh;
	class ArrowShader* as;
	class ItemList* il;
	class OptionShader* os;
	class ManyTexture* mt;
	class CharDrawer* cd;
	class Sounder* sounder;
	class Camera cam;

	std::vector<s_gridItemData> items;

	const char numX = 4;
	const char numY = 8;
	const float leftX = -80.0;
	const float xSize = numX * 9.0f + 1.0;
	const float ySize = 72.0;
	const char totalNum = numX * numY;

	char selectedItemIndex = -1;
	char lastHoverIndex = -1;

	float itemRatio = 0.0;
	bool itemDir = 0;
	int startAlloc;
	
	unsigned int switchSound;
	unsigned int clickSound;
	unsigned int bopSound;
	unsigned int removeSound;

	Overlayer(Game* game);

	static void maper(void* ptr);
	static void draw(VkCommandBuffer cmdBuff, void* ptr);

	void addItem(char* name, char* fileName, levelObject objectType, glm::vec2 gridRenderTranslation=glm::vec2(0.0, 0.0), int numTextureObjects=1,
		glm::ivec2 gridScale=glm::ivec2(1, 1), float gridRenderScale=1.0, bool canRotate=false, int requiredSideFill=-1);
	void update();
};