#include <Game.h>
#include <TestCaseIncludes.h>
#include <Sounder.h>
#include <Leveler.h>
#include <CharDrawer.h>

Game* Game::object;

Game::Game()
{
    Game::object = this;

	isPaused = false;
	tick = false;
	frame = 0;
    deltaTime = 0.0;
    lastTime = 0.0;
    gameTime = 0.0;
    testCase = nullptr;
    pWorld = nullptr;

    contactListener = nullptr;

	IOHelper::getAssetsFolder();

	thinDrawer = new ThinDrawer(pWorld);

	sounder = new Sounder();

    new CharDrawer(thinDrawer, "Japanese.ttf");

    gameStater = &Game::LevelerState;
}

void Game::LevelerState()
{
    leveler = new Leveler(this);

    lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(thinDrawer->wh->window))
    {
        frame++;

        deltaTime = glfwGetTime() - lastTime;
        gameTime += deltaTime;
        lastTime = glfwGetTime();

        sounder->update();

        thinDrawer->gameTime = gameTime;
        thinDrawer->renderLoop();
    }

    gameStater = nullptr;
}

void Game::TestState()
{
    testCase = newTest(this, 8);

    lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(thinDrawer->wh->window))
    {
        frame++;

        if ((!isPaused || tick) && testCase)
        {
            deltaTime = glfwGetTime() - lastTime;
            gameTime += deltaTime;
            lastTime = glfwGetTime();

            testCase->onUpdate();
        }
        else
        {
            deltaTime = 0.0;
            lastTime = glfwGetTime();
        }

        sounder->update();

        if (testCase)
        {
            testCase->imguiRender();

            testCase->keyHandler(thinDrawer->wh);

            if (testCase->shouldGetNewTest)
            {
                testCase->destroy();
                int nextTestCase = testCase->testCaseItem + 1;
                delete testCase;
                testCase = newTest(this, nextTestCase);
            }
        }

        thinDrawer->gameTime = gameTime;
        thinDrawer->renderLoop();
    }
    
    gameStater = nullptr;
}

void Game::loop()
{
    while (gameStater)
    {
        (this->*gameStater)();
    }
}