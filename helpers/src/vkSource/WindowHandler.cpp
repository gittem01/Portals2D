#include <WindowHandler.h>
#include <Camera.h>
#include <backends/imgui_impl_sdl3.h>
#include "ThinDrawer.h"
#include "SDL3/SDL_events.h"

WindowHandler::WindowHandler(Camera* cam)
{
    this->cam = cam;
    this->massInit();
}

void WindowHandler::handleMouseData()
{
    this->mouseData[5] = 0;
    this->trackpadData[0] = 0;
    this->trackpadData[1] = 0;
    for (int i = 2; i < 5; i++)
    {
        if (this->mouseData[i] == 2)
        {
            this->mouseData[i] = 1;
        }
    }
    if (lastMousePos[0] == mouseData[0] && lastMousePos[1] == mouseData[1])
    {
        mouseData[6] = 0;
    }
    else
    {
        mouseData[6] = 1;
    }
}

void WindowHandler::handleKeyData()
{
    for (int i : newPressIndices)
    {
        keyData[i] = 1;
    }

    for (int i : repeatClears)
    {
        repeatKeys[i] = 0;
    }

    newPressIndices.clear();
	repeatClears.clear();
}

bool WindowHandler::handleEvents()
{
    //SDL_StartTextInput(window); // to enable SDL_EVENT_TEXT_INPUT events
    
    text = "";

    SDL_Event evnt;
    while (SDL_WaitEventTimeout(&evnt, 1))
    {
        if (ImGui::GetDrawData()) ImGui_ImplSDL3_ProcessEvent(&evnt);

        switch (evnt.type)
        {
        case (SDL_EVENT_MOUSE_MOTION):
            mouseData[0] = (int)evnt.motion.x;
            mouseData[1] = (int)evnt.motion.y;
			lastMouseMovementTime = ThinDrawer::object->gameTime;
            break;
        case (SDL_EVENT_TEXT_INPUT):
            {
				text = std::string(evnt.text.text);
            }
            break;
		case (SDL_EVENT_MOUSE_BUTTON_DOWN):
			mouseData[evnt.button.button + 1] = 2;
			break;
		case (SDL_EVENT_MOUSE_BUTTON_UP):
			mouseData[evnt.button.button + 1] = 0;
			break;
		case (SDL_EVENT_MOUSE_WHEEL):
			mouseData[5] = evnt.wheel.y;

			trackpadData[0] = evnt.wheel.x;
			trackpadData[1] = evnt.wheel.y;
			break;
		case (SDL_EVENT_KEY_DOWN):
            if (evnt.key.repeat)
            {
                repeatKeys[evnt.key.scancode] = 1;
				repeatClears.push_back(evnt.key.scancode);
                break;
            }
            
			keyData[evnt.key.scancode] = 2;
			newPressIndices.push_back(evnt.key.scancode);
			break;
		case (SDL_EVENT_KEY_UP):
			keyData[evnt.key.scancode] = 0;
			break;
		case (SDL_EVENT_WINDOW_FOCUS_LOST):
            for (int i = 2; i < std::size(mouseData); i++)
            {
                mouseData[i] = 0;
            }

            trackpadData[0] = 0; trackpadData[1] = 0;

            break;
		case (SDL_EVENT_WINDOW_RESIZED):
        {
            windowSizes.x = evnt.window.data1;
			windowSizes.y = evnt.window.data2;

            if (windowSizes.x / windowSizes.y > 2.0f)
            {
				windowSizes.x = windowSizes.y * 2.0f;
				SDL_SetWindowSize(window, windowSizes.x, windowSizes.y);
            }
			else if (windowSizes.y / windowSizes.x > 1.0f)
			{
				windowSizes.y = windowSizes.x;
				SDL_SetWindowSize(window, windowSizes.x, windowSizes.y);
			}

            for (Camera* cam : Camera::cameras)
            {
                float ratio = (float)windowSizes.y / windowSizes.x;

                cam->defaultYSides = glm::vec2(-cam->baseX * ratio * 0.5f, cam->baseX * ratio * 0.5f);

                cam->xSides = cam->defaultXSides / (cam->zoom * cam->zoom) + cam->pos.x;
                cam->ySides = cam->defaultYSides / (cam->zoom * cam->zoom) + cam->pos.y;
                cam->updateOrtho();
            }
        }
			break;
		case (SDL_EVENT_QUIT):
			return true;
        default:
            break;
        }
    }

	return false;
}

bool WindowHandler::looper()
{
    handleMouseData();
    handleKeyData();

    double t0 = SDL_GetTicksSec();

    bool done = handleEvents();

	//printf("Time passed: %f\n", (SDL_GetTicksSec() - t0) * 1000.0);

    return done;
}

void WindowHandler::massInit()
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        printf("!SDL_Init()\n");
        exit(1);
    }

    window = SDL_CreateWindow("Portal2D", windowSizes.x, windowSizes.y,
        SDL_WINDOW_VULKAN | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN);
    if (!window)
    {
        printf("Could not create window\n");
        SDL_Quit();
        exit(1);
    }

    int x1, y1, x2, y2;
    SDL_GetWindowSize(window, &x1, &y1);
    SDL_GetWindowSizeInPixels(window, &x2, &y2);

    dpiScaling = std::round(x2 / x1);
    
    mouseData[0] = windowSizes.x / 2.0f;
    mouseData[1] = windowSizes.y / 2.0f;
}