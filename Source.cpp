#include "TestCaseIncludes.h"
#include <Sounder.h>

int main(void)
{
    IOHelper::getAssetsFolder();

    Sounder* s = new Sounder();

    thinDrawer = new ThinDrawer(pWorld);

    testCase = newTest(thinDrawer, 8);

    while (!glfwWindowShouldClose(thinDrawer->wh->window))
    {
        frame++;

        if (!isPaused || tick)
        {
            testCase->onUpdate();
        }
        else
        {
            thinDrawer->pDeltaTime = 0.0;
        }

        s->update();

        testCase->imguiRender();

        thinDrawer->renderLoop();

        testCase->keyHandler(thinDrawer->wh);

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
