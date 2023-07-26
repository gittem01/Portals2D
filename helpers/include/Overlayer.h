#pragma once

#include <vulkan/vulkan.h>
#include <Camera.h>

class Overlayer
{
public:
	class Game* game;
	class ThinDrawer* thinDrawer;
	class WindowHandler* wh;
	class ArrowShader* as;
	class ItemList* il;
	class Camera cam;

	Overlayer(Game* game);

	static void maper(void* ptr);
	static void draw(VkCommandBuffer cmdBuff, void* ptr);

	static void windowSizeEventCallback(GLFWwindow*, int, int);

	void update();
};