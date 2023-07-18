#pragma once

class Game
{
public:
	static Game* object;
	bool isPaused;
	bool tick;
	int frame;

	double gameTime;
	double deltaTime;
	double lastTime;

	class PortalWorld* pWorld;
	class mouseJointHandler* mjh;
	class ContactListener* contactListener;
	class ThinDrawer* thinDrawer;
	class Sounder* sounder;

	class Leveler* leveler;

	void(Game::*gameStater)(void);

	Game();
	
	void loop();

	void LevelerState();
	void TestState();
};