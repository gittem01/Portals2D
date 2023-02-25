#include "TestCaseIncludes.h"

int main(void)
{
    thinDrawer = new ThinDrawer(pWorld);

    testCase = newTest(thinDrawer, 8);

    while (!glfwWindowShouldClose(thinDrawer->wh->window))
    {
        frame++;

        testCase->imguiRender();

        if (!isPaused || tick)
        {
            testCase->onUpdate();
        }

        thinDrawer->renderLoop();

        keyHandler(thinDrawer->wh);

        if (testCase->shouldGetNewTest)
        {
            testCase->destroy();
            int nextTestCase = testCase->testCaseItem + 1;
            delete testCase;
            testCase = newTest(thinDrawer, nextTestCase);
        }
    }

    return 0;
}

#ifdef ENABLE_WINMAIN
    #include "windows.h"
    int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR szCmdLine, int iCmdShow)
    {
        return main();
    }
#endif
