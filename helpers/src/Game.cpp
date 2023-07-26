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

    new CharDrawer(thinDrawer, "Roboto-Regular.ttf");

    gameStater = &Game::LevelerState;

    glfwShowWindow(thinDrawer->wh->window);
}

void Game::LevelerState()
{
    leveler = new Leveler(this);

    thinDrawer->whiteWallShader->wallCount = 0;

    lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(thinDrawer->wh->window))
    {
        thinDrawer->wh->looper();
        thinDrawer->wh->cam->update();
        if (thinDrawer->wh->keyData[GLFW_KEY_G] == 2)
        {
            float sz = 1.5f;
            for (int i = -50; i < 50; i++)
            {
                for (int j = -50; j < 50; j++)
                {
                    float randVal11 = (((float)rand() / RAND_MAX) - 0.5f) * 0.4f;
                    float randVal22 = (((float)rand() / RAND_MAX)) * 0.0f;
                    s_wallUbo_w wallUbo;
                    wallUbo.posSize = glm::vec4(i * sz, j * sz, sz, sz);
                    wallUbo.bgColour = glm::vec4(0.4f + randVal11, 0.4f + randVal11, 0.4f + randVal11, 1.0f);
                    wallUbo.edgeColour = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
                    wallUbo.dotColour = glm::vec4(0.1f + randVal11, 0.1f + randVal11, 0.1f + randVal11, 1.0f);
                    wallUbo.data = glm::vec4(0.1f + randVal22, 0.06f, 2.0f, 1.0f);
                    wallUbo.data2 = glm::vec4(0.0, 0.0, 0.8f, 0.0f);

                    void* pData;
                    CHECK_RESULT_VK(vkMapMemory(thinDrawer->logicalDevice, thinDrawer->whiteWallShader->uniformBuffers[0]->memory,
                        thinDrawer->whiteWallShader->wallCount * sizeof(s_wallUbo_w), sizeof(s_wallUbo_w), 0, (void**)&pData));
                    memcpy(pData, &wallUbo, sizeof(s_wallUbo_w));
                    vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->whiteWallShader->uniformBuffers[0]->memory);
                    thinDrawer->whiteWallShader->wallCount++;
                }
            }
        }
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