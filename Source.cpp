#include "TestCaseIncludes.h"

int main(void)
{
    WindowPainter* wh = new WindowPainter(nullptr);
    Camera* cam = new Camera(glm::vec2(0, 0), wh);
    cam->zoom = 0.75f;
    wh->cam = cam;

    TestCase1* testCase = new TestCase1(wh);

    bool done = false;
    long sleepTime = 20; // millisecond

    while (!done)
    {
        frame++;

        done = wh->looper();

        cam->update();
        keyHandler(wh);

        if (!isPaused || tick) {
            glClear(GL_COLOR_BUFFER_BIT);

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            testCase->onUpdate();

            renderer->dotRender();

            renderer->render();
            if (pWorld){
                drawer->drawWorld(pWorld);
            }
            
            testCase->onDraw();

            //pWorld->DebugDraw();

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapInterval(1);

            glfwSwapBuffers(wh->window);

            if (pWorld && wh->keyData[GLFW_KEY_R] == 2){
                for (int i = 0; i < pWorld->portals.size(); i++){
                    pWorld->destroyPortal(pWorld->portals.at(i));
                    i--;
                }
                
                delete mjh;
                delete pWorld;

                pWorld = NULL;
                renderer->reset();

                delete testCase;
                testCase = new TestCase1(wh);
            }
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
        }
    }

    return 0;
}

#ifdef ENABLE_WINMAIN
    #include "windows.h"
    int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR szCmdLine, int iCmdShow){

        return main();
    }
#endif
