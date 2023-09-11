#include <Game.h>
#include <TestCaseIncludes.h>
#include <Sounder.h>
#include <Leveler.h>

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

    gameStater = &Game::TestState;

    glfwShowWindow(thinDrawer->wh->window);
}

void Game::LevelerState()
{
    leveler = new Leveler(this);

    lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(thinDrawer->wh->window))
    {
        thinDrawer->wh->looper();
        thinDrawer->wh->cam->update();

        frame++;

        deltaTime = glfwGetTime() - lastTime;
        gameTime += deltaTime;
        lastTime = glfwGetTime();

        leveler->update();
        sounder->update();

        thinDrawer->gameTime = gameTime;
        thinDrawer->renderLoop();
    }

    gameStater = nullptr;
}

void Game::TestState()
{
    testCase = newTest(this, 8);

    thinDrawer->whiteWallShader->testFill();
    thinDrawer->blackWallShader->testFill();
    thinDrawer->wallLight->testFill();
    thinDrawer->diagonalLiner->testFill();

    lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(thinDrawer->wh->window))
    {
        thinDrawer->wh->looper();
        thinDrawer->wh->cam->update();

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