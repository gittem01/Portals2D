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

	Game();
	
	void loop();
};