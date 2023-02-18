#include "TestCaseIncludes.h"
#include "ThinDrawer.h"

ThinDrawer* thinDrawer;

int main(void)
{
    thinDrawer = new ThinDrawer(pWorld);

    testCase = newTest(thinDrawer->wh, 1);

    while (!glfwWindowShouldClose(thinDrawer->wh->window))
    {
        frame++;

        thinDrawer->renderLoop();

        if (!isPaused || tick) {
            testCase->onUpdate();
        }

        keyHandler(thinDrawer->wh);

        if (testCase->shouldGetNewTest){
            int nextTestCase = testCase->testCaseItem + 1;
            delete testCase;
            testCase = newTest(thinDrawer->wh, nextTestCase);
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
