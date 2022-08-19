#include "TestCaseIncludes.h"

int main(void)
{
    WindowPainter* wh = new WindowPainter(nullptr);
    Camera* cam = new Camera(glm::vec2(0, 0), wh);
    cam->zoom = 0.75f;
    wh->cam = cam;

    testCase = newTest(wh, 1);
    bool done = false;

    while (!done)
    {
        frame++;

        done = wh->looper();
        cam->update();

        glClear(GL_COLOR_BUFFER_BIT);

        if (!isPaused || tick) {
            testCase->onUpdate();
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        renderer->dotRender();
        renderer->render();

        if (pWorld){
            drawer->drawWorld(pWorld);
        }
        
        testCase->onDraw();

        //pWorld->DebugDraw();

        keyHandler(wh);
        testCase->debugDraw();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapInterval(1);

        glfwSwapBuffers(wh->window);

        if (testCase->shouldGetNewTest){
            int nextTestCase = testCase->testCaseItem + 1;
            delete testCase;
            testCase = newTest(wh, nextTestCase);
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
