#include "Menu.h"
#include "ThinDrawer.h"
#include "Leveler.h"
#include "Overlayer.h"

#define INFINITY_CHAR "\xE2\x88\x9E"

Button::Button()
{
	bools.reset();
}

Menu::Menu()
{
	td = ThinDrawer::object;
	ol = Leveler::object->ol;

	auto lines = IOHelper::loadLines("menu.txt");
	buttonTexts = IOHelper::mapVector(lines);

	buttons.reserve(200);

	openCloseSpeed = 3.0f;
}

void Menu::initDefaults()
{
	buttons.clear();

	lastMouseTime = -100.0;
	activationTime = 0.0;
	upCamTime = 0.0f;
	lastDataSaveTime = 0.0;
	upMargin = -20.0f;
	currentY = 0.0f;
	lastMousePos = glm::vec2(-10000.0f, -10000.0f);
	menuCallTime = td->gameTime;

	hoverButton = nullptr;
	clickButton = nullptr;
	activeButton = nullptr;
	qButton = nullptr;
	soundButton = nullptr;
	loadButton = nullptr;
	newEditorButton = nullptr;
	exitButton = nullptr;
	keyAssignmentButton = nullptr;
}

void Menu::renderInit()
{
	td->lateDrawFuncs.push_back({ draw, this });
	td->memMapFuncs.push_back({ Overlayer::camMap, this });

	state = true;
}

void Menu::renderDestroy()
{
	td->eraseFromRenderFunctions(td->lateDrawFuncs, this);
	td->eraseFromMaperFunctions(td->memMapFuncs, this);
}

void Menu::initPlay(glm::vec4 clr)
{
	std::vector<std::string> allTexts = buttonTexts["<play_" + std::to_string(ol->cd->selectedLanguageIndex) + ">"];
	int texIndex = 0;

	Button* b = addButton(glm::vec2(0.0f, currentY), glm::vec2(50.0f, 6.0f), allTexts[texIndex++], clr);
	b->bools.set(INTERACTABLE); b->bools.set(RENDER_ENABLED); b->bools.set(MOVE_NEXT);
	b->openYMult = 40.0f;
	b->baseAlpha = clr.w;
	currentY += 8.0f;

	float levelButtonSize = 6.9f;
	float lastX = 50.0f - levelButtonSize;
	float firtsX = levelButtonSize;

	int numX = 6;
	float yMult = (float)lastX / (numX - 1);
	bool done = false;
	int y;
	Button* playButton = b;
	for (y = 0; !done; y++)
	{
		for (int x = 0; x < numX; x++)
		{
			int level = x + y * numX + 1;

			if (level >= Game::maxLevel)
			{
				done = true;
				break;
			}

			std::string c = std::to_string(level);

			float blueAmount = (x + 1.0f) / numX;
			float redAmount = (y + 1.0f) / 6.0f;
			Button* b = addButton(glm::vec2(-25.0f + levelButtonSize * 0.5f + lastX * ((float)x / (numX - 1)),
				-y * yMult), glm::vec2(levelButtonSize), c, glm::vec4(redAmount, 0.5f, blueAmount, 0.2f));
			b->baseAlpha = clr.w;
			if (level <= Game::lastLevel)
			{
				b->bools.set(INTERACTABLE);
			}
			else
			{
				b->data.colour *= 0.2f;
			}

			b->alphaMult = 0.0;
			playButton->dependentButtons.push_back(b);
		}
	}

	playButton->openYMult = yMult * y;
	playButton->openYAdder = 0.05f;
}

void Menu::initLeveler(glm::vec4 clr)
{
	std::vector<std::string> allTexts = buttonTexts["<leveler_" + std::to_string(ol->cd->selectedLanguageIndex) + ">"];
	int texIndex = 0;

	Button* b = addButton(glm::vec2(0.0f, currentY), glm::vec2(50.0f, 6.0f), allTexts[texIndex++], clr);
	b->bools.set(INTERACTABLE); b->bools.set(RENDER_ENABLED); b->bools.set(MOVE_NEXT);
	b->openYMult = 0.0f;
	b->baseAlpha = clr.w;
	currentY += 8.0f;

	Button* editorButton = b;
	newEditorButton = &buttons[buttons.size() - 1] + 1;
	loadButton = newEditorButton + 1;
	for (int i = 0; i < 3; i++)
	{
		Button* b = addButton(glm::vec2(0.0f, -i * 8.0f), glm::vec2(50.0f, 6.0f), allTexts[texIndex++], glm::vec4(0.0, 0.5, 1.0, 0.2));
		b->bools.set(INTERACTABLE);
		b->alphaMult = 0.0;
		b->baseAlpha = clr.w;
		editorButton->dependentButtons.push_back(b);
	}

	editorButton->openYMult = 24.0f;
	editorButton->openYAdder = 0.5f;

	loadButton->bools.set(NO_INTERACTION);
}

void Menu::initSettings(glm::vec4 clr)
{
	std::vector<std::string> allTexts = buttonTexts["<settings_" + std::to_string(ol->cd->selectedLanguageIndex) + ">"];
	int texIndex = 0;

	Button* b = addButton(glm::vec2(0.0f, currentY), glm::vec2(50.0f, 6.0f), allTexts[texIndex++], clr);
	b->bools.set(INTERACTABLE); b->bools.set(RENDER_ENABLED); b->bools.set(MOVE_NEXT);
	b->openYMult = 50.0f;
	b->baseAlpha = clr.w;
	currentY += 8.0f;

	Button* optionsButton = b;

	std::vector<float> soundValues = { Game::object->sounder->generalVolume, Game::object->sounder->musicVolume, Game::object->sounder->effectVolume };

	soundButton = &buttons[buttons.size() - 1] + 3;
	for (int i = 0; i < soundValues.size(); i++)
	{
		Button* b = addButton(glm::vec2(0.0f, -i * 5), glm::vec2(50.0f - 0.75f, 3.0f), allTexts[texIndex++],
			glm::vec4(0.3, 0.6, 1.0, 1.0f), glm::vec2(1.0f, 1.5f));
		b->bools.set(TEXT_LEFT);
		b->alphaMult = 0.0; b->baseAlpha = clr.w * 0.5;
		optionsButton->dependentButtons.push_back(b);

		b = addButton(glm::vec2(10.0f, -i * 5 - 1.5f), glm::vec2(25.0f, 0.0f), "", glm::vec4(0.0f), glm::vec2(3.0f));
		b->bools.set(INTERACTABLE);
		b->alphaMult = 0.0; b->baseAlpha = 1.0;
		optionsButton->dependentButtons.push_back(b);

		float minX = b->data.posSize.x - b->data.posSize.z * 0.5f;
		float maxX = b->data.posSize.x + b->data.posSize.z * 0.5f;

		float bx = minX + (maxX - minX) * soundValues[i];

		b = addButton(glm::vec2(bx, -i * 5 - 1.5f), glm::vec2(0.0f, 0.0f), "", glm::vec4(1.0f), glm::vec2(3.0f));
		b->bools.set(INTERACTABLE); b->type = BUTTON_CIRCLE;
		b->alphaMult = 0.0; b->baseAlpha = clr.w;
		optionsButton->dependentButtons.push_back(b);
	}

	glm::vec4 clrs[6] = { glm::vec4(1.0f, 0.6f, 0.0f, 1.0f),
		glm::vec4(0.0f, 0.5f, 1.0f, 1.0f), glm::vec4(1.0f, 0.6f, 0.0f, 1.0f), glm::vec4(0.0f, 0.5f, 1.0f, 1.0f),
		glm::vec4(0.0f, 0.5f, 1.0f, 1.0f), glm::vec4(0.0f, 0.5f, 1.0f, 1.0f) };

	SDL_Scancode scanCodes[6] = { Game::rotateKey, Game::jumpKey, Game::holdKey, Game::leftKey, Game::downKey, Game::rightKey };

	qButton = &buttons[buttons.size() - 1] + 3;
	float bSize = 46.0f / 3;
	for (int y = 0; y < 2; y++)
	{
		for (int x = 0; x < 3; x++)
		{
			Button* b = addButton(glm::vec2(-25.0f + bSize * 0.5f + x * (bSize + 2.0f), -y * 12.5f - 15.0f),
				glm::vec2(bSize - 0.75f, 10.0f), "", clrs[y * 3 + x], glm::vec2(1.0f, 1.5f));
			b->alphaMult = 0.0; b->baseAlpha = clr.w * 0.3;
			optionsButton->dependentButtons.push_back(b);

			glm::vec2 pos = glm::vec2(b->data.posSize.x, b->data.posSize.y + 0.5f);
			b = addButton(pos, glm::vec2(0.0f, 4.0f), allTexts[texIndex++], glm::vec4(0.0f));
			b->alphaMult = 0.0; b->baseAlpha = 0.0f;
			optionsButton->dependentButtons.push_back(b);

			const char* text = SDL_GetScancodeName(scanCodes[y * 3 + x]);

			pos.y -= 5.5f;
			b = addButton(pos, glm::vec2(bSize * 0.8f, 4.0f), text, glm::vec4(1.0f), glm::vec2(1.5f));
			b->alphaMult = 0.0; b->baseAlpha = clr.w * 0.3;
			b->bools.set(INTERACTABLE);
			optionsButton->dependentButtons.push_back(b);
		}
	}

	for (int i = 0; i < 3; i++)
	{
		b = addButton(glm::vec2(0.0f, -39.5f - i * 5.0f),
			glm::vec2(50.0f - 0.75f, 3.0f), allTexts[texIndex++], glm::vec4(0.5f, 0.0f, 0.5f, 1.0f), glm::vec2(1.0f, 1.5f));
		b->bools.set(TEXT_LEFT);
		b->alphaMult = 0.0; b->baseAlpha = clr.w * 0.6;
		optionsButton->dependentButtons.push_back(b);

		b = addButton(glm::vec2(22.25f, -39.5f - i * 5.0f - 1.5f), glm::vec2(0.0f, 0.0f), "", glm::vec4(0.0f), glm::vec2(3.0f));
		b->bools.set(INTERACTABLE);
		b->alphaMult = 0.0; b->baseAlpha = 1.0f;
		optionsButton->dependentButtons.push_back(b);

		b = addButton(glm::vec2(22.25f, -39.5f - i * 5.0f - 1.5f), glm::vec2(0.0f, 0.0f), "", glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), glm::vec2(2.0f, 2.5f));
		b->alphaMult = 0.0; b->baseAlpha = 0.6f;
		optionsButton->dependentButtons.push_back(b);
	}

	b = addButton(glm::vec2(0.0f, -39.5f - 3 * 5.0f),
		glm::vec2(50.0f - 0.75f, 3.0f), allTexts[texIndex++], glm::vec4(0.5f, 0.0f, 0.5f, 1.0f), glm::vec2(1.0f, 1.5f));
	b->bools.set(TEXT_LEFT);
	b->alphaMult = 0.0; b->baseAlpha = clr.w * 0.6;
	optionsButton->dependentButtons.push_back(b);

	std::string frameString = Game::limitFramerate < 1001 ? std::to_string(Game::limitFramerate) : INFINITY_CHAR;

	b = addButton(glm::vec2(19.0f, -39.5f - 3 * 5.0f - 0.5f), glm::vec2(8.0f, 2.0f), frameString, glm::vec4(0.0f), glm::vec2(1.5f, 2.0f));
	b->bools.set(INTERACTABLE);
	b->alphaMult = 0.0; b->baseAlpha = 1.0f;
	addTextToButton(*b, frameString, Game::limitFramerate == 1001 ? 5.0f : 2.0f);
	optionsButton->dependentButtons.push_back(b);

	maxFramerateButton = b;
	vsyncButton = b - 3;
	borderlessButton = b - 6;
	fullScreenButton = b - 9;
	
	auto flags = SDL_GetWindowFlags(td->wh->window);

	(fullScreenButton + 1)->baseAlpha = (flags & SDL_WINDOW_FULLSCREEN) ? 1.0f : 0.0f;
	(borderlessButton + 1)->baseAlpha = (flags & SDL_WINDOW_BORDERLESS) ? 1.0f : 0.0f;
	(vsyncButton + 1)->baseAlpha = td->swapChain->enableVsync ? 1.0f : 0.0f;

	b = addButton(glm::vec2(0.0f, -39.5f - 4 * 5.0f),
		glm::vec2(50.0f - 0.75f, 3.0f), "", glm::vec4(0.0f, 0.5f, 0.5f, 1.0f), glm::vec2(1.0f, 1.5f));
	addTextToButton(*b, allTexts[texIndex++], 1.5f);
	b->bools.set(INTERACTABLE);
	b->alphaMult = 0.0; b->baseAlpha = clr.w * 0.6;
	languageButton = b;
	optionsButton->dependentButtons.push_back(b);

	b = addButton(glm::vec2(0.0f, -39.5f - 5 * 5.0f),
		glm::vec2(50.0f - 0.75f, 3.0f), "", glm::vec4(1.0f, 1.0f, 0.0f, 1.0f), glm::vec2(1.0f, 1.5f));
	addTextToButton(*b, allTexts[texIndex++], 1.5f);
	b->bools.set(INTERACTABLE);
	b->alphaMult = 0.0; b->baseAlpha = clr.w * 0.6;
	gpuButton = b;
	optionsButton->dependentButtons.push_back(b);

	optionsButton->openYMult = 69.5f;
	optionsButton->openYAdder = 0.05f;
}

Button* Menu::initWithName(std::string name, glm::vec4 clr)
{
	std::vector<std::string> allTexts = buttonTexts["<" + name + "_" + std::to_string(ol->cd->selectedLanguageIndex) + ">"];

	Button* b = addButton(glm::vec2(0.0f, currentY), glm::vec2(50.0f, 6.0f), allTexts[0], clr);
	b->bools.set(INTERACTABLE); b->bools.set(RENDER_ENABLED); b->bools.set(MOVE_NEXT);
	b->openYMult = 0.0f;
	b->baseAlpha = clr.w;
	b->strText = name;

	currentY += 8.0f;

	return b;
}

void Menu::initCredits(glm::vec4 clr)
{
	std::vector<std::string> allTexts = buttonTexts["<credits_" + std::to_string(ol->cd->selectedLanguageIndex) + ">"];

	Button* b = addButton(glm::vec2(0.0f, currentY), glm::vec2(50.0f, 6.0f), allTexts[0], clr);
	b->bools.set(INTERACTABLE); b->bools.set(RENDER_ENABLED); b->bools.set(MOVE_NEXT);
	b->openYMult = 10.0f;
	b->baseAlpha = clr.w;
	
	currentY += 8.0f;
}

void Menu::addTextToButton(Button& b, std::string text, float sizeMult)
{
	float texSize = b.data.posSize.w * 0.8f * sizeMult;
	float camY = ol->cam.defaultYSides.y * 2.0f;
	float screenY = ol->cd->creationHeight;
	int reqTexSize = (int)(texSize * screenY / camY) + 8;

	b.strText = text;
	float x;
	b.text.charDatas = ol->cd->getRenderData(text, reqTexSize, x);
	b.text.color = glm::vec4(1.0f);

	if (x * texSize > (b.data.posSize.z + b.data.data.x) && b.data.colour.w > 0.0001f)
	{
		float ratio = x * texSize / (b.data.posSize.z + b.data.data.x);
		texSize /= ratio;
	}

	b.text.posScale = glm::vec4(b.data.posSize.x - x * texSize * 0.5f, b.data.posSize.y, texSize, x);
}

Button* Menu::addButton(glm::vec2 pos, glm::vec2 size, std::string text, glm::vec4 color, glm::vec2 data)
{
	Button b = Button();
	b.data.posSize = glm::vec4(pos, size);
	b.data.colour = color;
	b.data.data = glm::vec4(data, 1.0f, 1.0f);
	b.generateVertices();

	if (!text.empty())
	{
		addTextToButton(b, text);
	}

	b.baseX = pos.x;
	b.baseY = pos.y;

	buttons.push_back(b);

	return &buttons.back();
}

void Menu::update()
{
	renderDatas.clear();

	glm::vec2 mousePos = ol->cam.getMouseCoords();
	double timeNow = ThinDrawer::object->gameTime;
	double dt = ThinDrawer::object->deltaTime;

	float camUp = ol->cam.defaultYSides.y;

	double mouseTimeDiff = timeNow - lastMouseTime;

	glm::vec2 diffVec = mousePos - lastMousePos;
	float diffLen = glm::dot(diffVec, diffVec);

	float val01;
	const double maxVal = 0.25;
	if (activeButton)
	{
		double timeDiff = std::min(timeNow - activationTime, maxVal) / maxVal;
		val01 = timeDiff;
		activeButton->activeAmount = val01;

		Leveler::object->ol->cd->allCharDatas[maxFramerateButton->text.charDatas[0]].size_leftBotMargin.w = 0.0f;
	}
	else
	{
		val01 = 0.0f;
	}

	float yNow;
	if (upCamTime > 0.0 && activeButton && activeButton->dependentButtons.size() > 0)
	{
		double timeDiff = std::min(timeNow - upCamTime, maxVal) / maxVal;
		upMargin = std::max(upMargin - dt * 100.0, 4.0 - activeButton->baseY);

		yNow = camUp - upMargin;
	}
	else
	{
		double positiveTime = abs(upCamTime);
		double timeDiff = std::min(timeNow - positiveTime, maxVal) / maxVal;

		upMargin = std::min(upMargin + dt * 100.0, 20.0);

		yNow = camUp - upMargin;
	}

	hoverButton = nullptr;
	for (Button& button : buttons)
	{
		if (!button.bools[RENDER_ENABLED]) continue;
		
		if (button.bools[MOVE_NEXT])
		{
			button.data.posSize.y = yNow;
			button.text.posScale.y = yNow;

			yNow -= 8.0f + pow(button.activeAmount, 1.0) * button.openYMult;
		}

		if (button.activeAmount > 0.0)
		{
			for (Button* dependentButton : button.dependentButtons)
			{
				dependentButton->data.posSize.y = dependentButton->baseY + button.data.posSize.y -
					(button.data.posSize.w + dependentButton->data.posSize.w + 4.0) * 0.5;

				dependentButton->text.posScale.y = dependentButton->data.posSize.y;

				if (&button == activeButton)
				{
					dependentButton->alphaMult += dt * 4.0;
				}
				else
				{
					dependentButton->alphaMult -= dt * button.openYMult * 0.2;
				}

				dependentButton->text.color.w = std::min(dependentButton->alphaMult, 1.0f);

				if (dependentButton->bools[TEXT_LEFT])
				{
					dependentButton->text.posScale.x = dependentButton->data.posSize.x -
						dependentButton->data.posSize.z * 0.5f + 1.0f;
				}
				else
				{
					dependentButton->text.posScale.x = dependentButton->data.posSize.x -
						dependentButton->text.posScale.w * dependentButton->text.posScale.z * 0.5;
				}

				dependentButton->bools.set(RENDER_ENABLED);
			}

			button.activeAmount = button.activeAmount - dt * 2.0;

			if (button.activeAmount < 0.0)
			{
				button.activeAmount = 0.0;
				for (Button* dependentButton : button.dependentButtons)
				{
					dependentButton->bools.set(RENDER_ENABLED, false);
					dependentButton->alphaMult = 0.0f;
				}
			}
		}

		if (button.text.charDatas.size() > 0)
		{
			renderDatas.push_back(button.text);
		}

		if (button.bools[INTERACTABLE] && !Game::object->pop.active && clickButton != maxFramerateButton)
		{
			if (mouseTimeDiff < 0.1 && diffLen > 0.5f)
			{
				float len;
				if (button.rayCheck(b2Vec2(lastMousePos.x, lastMousePos.y), b2Vec2(mousePos.x, mousePos.y), &len) &&
					!(&button == clickButton - 1 && clickButton->type == BUTTON_CIRCLE))
				{
					button.lastHoverTime = lastMouseTime + mouseTimeDiff * len / diffLen;
				}
			}

			if (&button > hoverButton && button.hoverCheck(mousePos) && !(&button == clickButton - 1 && clickButton->type == BUTTON_CIRCLE))
			{
				hoverButton = &button;
			}
		}

		float hoverDiff = timeNow - button.lastHoverTime;
		button.data.data.y = button.data.data.x + 0.25f + std::max(0.35f - hoverDiff, 0.0f) * 3.0f;
		button.data.colour.w = (button.baseAlpha + std::max(0.35f - hoverDiff, 0.0f)) * std::clamp(button.alphaMult, 0.0f, 1.0f);
	}

	if (hoverButton)
	{
		hoverButton->lastHoverTime = timeNow;
	}

	if (td->wh->mouseData[2] == 2)
	{
		if (keyAssignmentButton)
		{
			keyAssignmentButton->data.colour.w = std::clamp(keyAssignmentButton->alphaMult, 0.0f, 1.0f);
			keyAssignmentButton->text.color = glm::vec4(1.0f);
			keyAssignmentButton = nullptr;
		}

		if (hoverButton)
		{
			windowClickPos.x = td->wh->mouseData[0];
			windowClickPos.y = td->wh->mouseData[1];

			if ((hoverButton + 1) < buttons.data() + buttons.size() && (hoverButton + 1)->type == BUTTON_CIRCLE)
			{
				clickButton = hoverButton + 1;
			}
			else
			{
				clickButton = hoverButton;
				if (clickButton == maxFramerateButton)
				{
					SDL_CaptureMouse(true);
					SDL_HideCursor();
					SDL_SetWindowRelativeMouseMode(td->wh->window, true);
				}
			}
		}
	}

	if (clickButton && td->wh->mouseData[2] == 0)
	{
		if (clickButton == hoverButton)
		{
			buttonPressed(clickButton);
		}

		clickButton->text.color = glm::vec4(1.0f, 1.0, 1.0, std::clamp(clickButton->alphaMult, 0.0f, 1.0f));

		if (clickButton == maxFramerateButton)
		{
			SDL_SetWindowRelativeMouseMode(td->wh->window, false);
			SDL_WarpMouseInWindow(td->wh->window, windowClickPos.x, windowClickPos.y);
			SDL_SetWindowMouseGrab(td->wh->window, false);
			SDL_CaptureMouse(false);
			SDL_ShowCursor();
		}

		clickButton = nullptr;
	}

	if ((clickButton == hoverButton || clickButton && clickButton->type == BUTTON_CIRCLE) && clickButton)
	{
		clickButton->data.colour.w = std::clamp(clickButton->alphaMult, 0.0f, 1.0f);
		clickButton->text.color = glm::vec4(0.0f, 0.0f, 0.0f, std::clamp(clickButton->alphaMult, 0.0f, 1.0f));
		clickButton->lastHoverTime = timeNow;

		if (clickButton->type == BUTTON_CIRCLE)
		{
			Button* clickSlider = clickButton - 1;
			float minX = clickSlider->data.posSize.x - clickSlider->data.posSize.z * 0.5f;
			float maxX = clickSlider->data.posSize.x + clickSlider->data.posSize.z * 0.5f;
			int lastPos = (int)(clickButton->data.posSize.x * 128.0f);
			clickButton->data.posSize.x = std::clamp(mousePos.x, minX, maxX);
			
			if ((int)(clickButton->data.posSize.x * 128.0f) != lastPos)
			{
				ALuint source = Sounder::object->playSound(ol->clickSound, 0.25);
				alSourcei(source, AL_SOURCE_RELATIVE, AL_TRUE);

				float ratio = (clickButton->data.posSize.x - minX) / (maxX - minX);
				int buttonIndex = (clickButton - soundButton) / 3;
				switch (buttonIndex)
				{
				case 0: Game::object->sounder->generalVolume = ratio; break;
				case 1: Game::object->sounder->musicVolume = ratio; break;
				case 2: Game::object->sounder->effectVolume = ratio; break;
				default: break;
				}
			}
		}
	}
	else if (clickButton)
	{
		clickButton->text.color = glm::vec4(1.0f);
	}

	float differ = 0.0f;
	if (clickButton == maxFramerateButton)
	{
		maxFramerateButton->text.color = glm::vec4(1.0f);

		differ = td->wh->mouseData[7] * 0.1f;
	}
	else if (hoverButton == maxFramerateButton)
	{
		differ += td->wh->mouseData[5];
	}

	if (abs(differ) > 0.000001f)
	{
		int frameRateNum;
		if (maxFramerateButton->strText == INFINITY_CHAR)
		{
			frameRateNum = 1001;
		}
		else
		{
			frameRateNum = atoi(maxFramerateButton->strText.c_str());
		}

		frameRateNum += differ * 10.0f;
		frameRateNum = std::clamp(frameRateNum, 30, 1001);

		Game::limitFramerate = frameRateNum;

		std::string str = frameRateNum == 1001 ? INFINITY_CHAR : std::to_string(frameRateNum);
		addTextToButton(*maxFramerateButton, str, frameRateNum == 1001 ? 5.0f : 2.0f);
	}

	if (keyAssignmentButton)
	{
		keyAssignmentButton->data.colour.w = 1.0f;
		keyAssignmentButton->text.color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

		for (int i = 0; i < SDL_SCANCODE_COUNT; i++)
		{
			if (td->wh->keyData[i] == 2)
			{
				const char* text = SDL_GetScancodeName((SDL_Scancode)i);
				addTextToButton(*keyAssignmentButton, text);

				int buttonIndex = (keyAssignmentButton - qButton) / 3;
				switch (buttonIndex)
				{
				case 0: Game::rotateKey = (SDL_Scancode)i; break;
				case 1: Game::jumpKey = (SDL_Scancode)i; break;
				case 2: Game::holdKey = (SDL_Scancode)i; break;
				case 3: Game::leftKey = (SDL_Scancode)i; break;
				case 4: Game::downKey = (SDL_Scancode)i; break;
				case 5: Game::rightKey = (SDL_Scancode)i; break;
				default: break;
				}

				keyAssignmentButton = nullptr;

				break;
			}
		}
	}

	if (timeNow - lastDataSaveTime > 0.25f)
	{
		Game::object->saveGameData();
		lastDataSaveTime = timeNow;
	}

	lastMousePos = mousePos;
	lastMouseTime = td->wh->lastMouseMovementTime;
}

void Menu::buttonPressed(Button* button)
{
	if (button->type == BUTTON_CIRCLE) return;
	if ((button + 1) < buttons.data() + buttons.size() && (button + 1)->type == BUTTON_CIRCLE) return;

	if (button >= qButton && button <= qButton + 15)
	{
		keyAssignmentButton = button;
		return;
	}

	if (button == languageButton)
	{
		Game::object->pop.init(languageButton->data, "languageRender", 0.5);
		return;
	}
	else if (button == gpuButton)
	{
		Game::object->pop.init(gpuButton->data, "gpuRender", 0.5);
		return;
	}

	if (button >= fullScreenButton && button <= vsyncButton)
	{
		Button* displayButton = button + 1;
		displayButton->baseAlpha = displayButton->baseAlpha > 0.99f ? 0.0f : 1.0f;

		uint64_t setMask = 0;

		if (button == fullScreenButton)
		{
			setMask = SDL_WINDOW_FULLSCREEN;
			if (displayButton->baseAlpha < 0.01f)
			{
				td->wh->setWindowSizes(0.8f);
				SDL_SetWindowPosition(td->wh->window,
					(td->wh->displayMode->w - td->wh->windowSizes.x) / 2,
					(td->wh->displayMode->h - td->wh->windowSizes.y) / 2);
			}
		}
		else if (button == borderlessButton)
		{
			setMask = SDL_WINDOW_BORDERLESS;
		}
		else if (button == vsyncButton)
		{
			td->swapChain->enableVsync = !td->swapChain->enableVsync;
		}

		if (displayButton->baseAlpha > 0.99f)
		{
			WindowHandler::windowCreateFlags |= setMask;
		}
		else
		{
			WindowHandler::windowCreateFlags &= ~setMask;
		}

		Game::object->singleTaskRegistry.push_back([] { ThinDrawer::object->surfaceRecreate(); });

		return;
	}
	else if (button == maxFramerateButton)
	{
		return;
	}

	if (!activeButton) upCamTime = td->gameTime;
	else
	{
		float i = 1.0f;
		for (int j = activeButton->dependentButtons.size() - 1; j >= 0; j--)
		{
			activeButton->dependentButtons[j]->alphaMult = std::min(i, activeButton->dependentButtons[j]->alphaMult);
			i += 0.05f;
		}
	}

	if (activeButton == button)
	{
		upCamTime = -td->gameTime;
		activeButton = nullptr;
	}
	else if (!button->bools[NO_INTERACTION])
	{
		activationTime = td->gameTime;
		activeButton = button;
		
		float i = 0.0f;
		for (Button* dependentButton : button->dependentButtons)
		{
			dependentButton->alphaMult = i;
			i -= button->openYAdder;
		}
	}

	if (button == exitButton)
	{
		Game::object->shouldQuit = true;
	}
	else if (button == newEditorButton)
	{
		s_menuButton initialButton = {
		glm::vec4(0.0f, 0.0f, 300.0f, 300.0f), // posSize
		glm::vec4(0.0f, 0.0f, 0.0f, 0.0), // clr
		glm::vec4(5.0f, 5.5f, 1.0f, 0.1f) }; // data
		Game::object->pop.init(initialButton, "darkness", 0.5);

		Game::object->pop.endFunction = [] {
			Game::object->gameStater = &Game::LevelerState;
		};
	}
	else if (button == loadButton)
	{
		Game::object->pop.init(loadButton->data, "loadRender", 0.5);
	}
}

float Menu::getDt01()
{
	float dt01;
	if (!state)
	{
		dt01 = 1.0 - std::min((td->gameTime - menuCloseTime) * openCloseSpeed, 1.0);
	}
	else
	{
		dt01 = std::min((td->gameTime - menuCallTime) * openCloseSpeed, 1.0);
	}

	return dt01;
}

void Menu::setNewState(bool newState)
{
	if (state == newState) return;

	if (newState)
	{
		float dt01 = getDt01();
		menuCallTime = td->gameTime - dt01 / openCloseSpeed;
	}
	else
	{
		float dt01 = getDt01();
		menuCloseTime = td->gameTime - (1.0 - dt01) / openCloseSpeed;
	}

	if (clickButton == maxFramerateButton)
	{
		SDL_SetWindowRelativeMouseMode(td->wh->window, false);
		SDL_WarpMouseInWindow(td->wh->window, windowClickPos.x, windowClickPos.y);
		SDL_SetWindowMouseGrab(td->wh->window, false);
		SDL_CaptureMouse(false);
		SDL_ShowCursor();
	}

	state = newState;
}

void Menu::draw(VkCommandBuffer cmdBuff, void* ptr)
{
	Menu* menu = (Menu*)ptr;

	ThinDrawer* td = ThinDrawer::object;
	Overlayer* ol = Leveler::object->ol;

	const uint32_t camOffset = ShaderBase::alignedSharedSize * (td->frameIndex + 8);

	vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, td->menuButton->pipeline);

	vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
		td->menuButton->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

	float dt01 = menu->getDt01();
	if (!menu->state && dt01 < 0.0001f)
	{
		Game::object->singleTaskRegistry.push_back([menu]
		{
			menu->renderDestroy();
		});
	}

	if (Game::object->gameStater != &Game::MenuState)
	{
		s_menuButton bgView;
		float sinAdd = sin(td->gameTime) * 0.05 + 0.05;
		bgView.colour = glm::vec4(0.0f, 0.0f, 0.0f, dt01);
		bgView.posSize = glm::vec4(0.0f, 0.0f, Leveler::object->ol->cam.xSides.y * 2.0f, Leveler::object->ol->cam.ySides.y * 2.0f);
		bgView.data = glm::vec4(0.0f, 0.0f, 1.05f + sinAdd, 0.1f);

		vkCmdPushConstants(cmdBuff, td->menuButton->pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_menuButton), &bgView);

		vkCmdDraw(cmdBuff, 6, 1, 0, 0);
	}

	for (Button& button : menu->buttons)
	{
		if (!button.bools[RENDER_ENABLED]) continue;

		float oldAlpha = button.data.colour.w;
		button.data.colour.w *= dt01;

		vkCmdPushConstants(cmdBuff, td->menuButton->pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_menuButton), &button.data);

		vkCmdDraw(cmdBuff, 6, 1, 0, 0);

		button.data.colour.w = oldAlpha;
	}

	std::vector<float> oldAlphas;

	if (dt01 != 1.0f)
	{
		oldAlphas.resize(menu->renderDatas.size());
		int i = 0;
		for (auto& lrd : menu->renderDatas)
		{
			oldAlphas[i++] = lrd.color.w;
			lrd.color.w *= dt01;
		}
	}

	auto temp = std::move(ol->cd->renderDatas);
	ol->cd->renderDatas = std::move(menu->renderDatas);

	bool clearTemp = ol->cd->clear;
	ol->cd->clear = false;
	CharDrawer::draw(cmdBuff, ol->cd);
	ol->cd->clear = clearTemp;

	menu->renderDatas = std::move(ol->cd->renderDatas);
	ol->cd->renderDatas = std::move(temp);

	if (dt01 != 1.0f)
	{
		int i = 0;
		for (auto& lrd : menu->renderDatas)
		{
			lrd.color.w = oldAlphas[i++];
		}
	}
}

void Button::generateVertices()
{
	glm::vec2 pos = glm::vec2(data.posSize.x, data.posSize.y);
	glm::vec2 size = (glm::vec2(data.posSize.z, data.posSize.w) + data.data.x) * 0.5f;

	vertices[0] = glm::vec2(-size.x, -size.y);
	vertices[1] = glm::vec2(+size.x, -size.y);
	vertices[2] = glm::vec2(+size.x, +size.y);
	vertices[3] = glm::vec2(-size.x, +size.y);
}

bool Button::hoverCheck(glm::vec2 pos)
{
	glm::vec2 size = (glm::vec2(data.posSize.z, data.posSize.w) + data.data.x) * 0.5f;

	if (type == BUTTON_RECTANGLE)
	{
		return data.hoverCheck(pos);
	}
	else if (type == BUTTON_CIRCLE)
	{
		float dist = glm::distance(pos, glm::vec2(data.posSize));
		if (dist <= size.x)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}

bool Button::rayCheck(b2Vec2 p21, b2Vec2 p22, float* len)
{
	float farthestDist = -100.0f;
	int intersectCount = 0;
	bool intersect = false;

	b2Vec2 pos = b2Vec2(data.posSize.x, data.posSize.y);

	for (int i = 0; i < 4; i++)
	{
		b2Vec2 p11 = b2Vec2(vertices[i].x, vertices[i].y) + pos;
		b2Vec2 p12 = b2Vec2(vertices[(i + 1) % 4].x, vertices[(i + 1) % 4].y) + pos;

		b2Vec2 intersectPoint = PortalWorld::getLineIntersection(p11, p12, p21, p22);

		float len1 = b2Dot(p12 - p11, intersectPoint - p11);
		float len2 = b2Dot(p22 - p21, intersectPoint - p21);

		float len3 = (p12 - p11).LengthSquared();
		float len4 = (p22 - p21).LengthSquared();

		bool innerIntersect = len3 >= len1 && len1 >= 0.0f && len4 >= len2 && len2 >= 0.0f;

		float length = (p21 - intersectPoint).Length();
		if (innerIntersect && length > farthestDist)
		{
			*len = length;
			farthestDist = length;
			intersect = true;
		}
	}

	return intersect;
}