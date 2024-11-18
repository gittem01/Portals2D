#pragma once

#include <vector>
#include <functional>
#include <mutex>
#include "SDL3/SDL.h"

class Game
{
public:

	static SDL_Scancode jumpKey;
	static SDL_Scancode leftKey;
	static SDL_Scancode rightKey;
	static SDL_Scancode downKey;
	static SDL_Scancode rotateKey;
	static SDL_Scancode holdKey;

	static int lastLevel;
	static int maxLevel;

	static Game* object;

	bool isPaused;
	bool tick;
	int frame;
	bool shouldQuit;

	double gameTime;
	double deltaTime;
	double lastTime;
	double dtMult;

	std::vector<std::function<void(void)>> singleTaskRegistry;

	class BaseTestCase* testCase;

	class PortalWorld* pWorld;
	class mouseJointHandler* mjh;
	class ContactListener* contactListener;
	class ThinDrawer* thinDrawer;
	class Sounder* sounder;

	class Leveler* leveler;
	class Menu* menu;

	void(Game::* gameStater)(void);
	void(Game::* lastState)(void) = nullptr;

	Game();
	
	void loadGameData();
	void saveGameData();

	void loop();

	void timeHandle();

	void LevelerState();
	void TestState();
	void MenuState();
};