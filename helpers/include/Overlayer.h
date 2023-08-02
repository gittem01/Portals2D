#pragma once

#include <vulkan/vulkan.h>
#include <Camera.h>
#include <Leveler.h>
#include <string>

typedef struct GridItemData
{
	std::string name;
	levelObject objectType;
	VkDescriptorSet texSet;
	float extraScale;
} s_gridItemData;

class Overlayer
{
public:
	class Game* game;
	class ThinDrawer* thinDrawer;
	class WindowHandler* wh;
	class ArrowShader* as;
	class ItemList* il;
	class ManyTexture* mt;
	class Sounder* sounder;
	class Camera cam;

	std::vector<s_gridItemData> items;

	const char numX = 2;
	const char numY = 8;
	const char totalNum = numX * numY;

	char selectedItemIndex = -1;
	char lastHoverIndex = -1;

	float itemRatio = 0.0;
	bool itemDir = 0;
	int startAlloc;
	
	unsigned int switchSound;
	unsigned int clickSound;

	Overlayer(Game* game);

	static void maper(void* ptr);
	static void draw(VkCommandBuffer cmdBuff, void* ptr);

	static void windowSizeEventCallback(GLFWwindow*, int, int);

	void addItem(char* name, char* fileName, levelObject objectType);
	void update();
};