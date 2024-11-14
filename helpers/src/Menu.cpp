#include "Menu.h"
#include "ThinDrawer.h"
#include "Leveler.h"
#include "Overlayer.h"

static std::vector<std::string> keys = { "ROTATE", "JUMP", "HOLD", "LEFT", "DOWN", "RIGHT" };

Button::Button()
{
	bools.reset();
}

Menu::Menu()
{
	td = ThinDrawer::object;
	ol = Leveler::object->ol;

	td->lateDrawFuncs.push_back({ draw, this });
	td->lateDrawFuncs.push_back({ CharDrawer::draw, ol->cd });

	td->memMapFuncs.push_back({ Overlayer::maper, nullptr });

	float camUp = ol->cam.defaultYSides.y;

	std::vector<std::string> baseButtons = { "PLAY", "LEVEL EDITOR", "OPTIONS", "CREDITS", "EXIT" };
	std::vector<glm::vec4> colors = {	glm::vec4(1.0, 1.0, 1.0, 0.2), glm::vec4(1.0, 1.0, 1.0, 0.2),
										glm::vec4(1.0, 1.0, 1.0, 0.2), glm::vec4(0.0, 1.0, 0.0, 0.2), glm::vec4(1.0, 0.0, 0.0, 0.2) };
	std::vector<float> yMults = { 40.0f, 0.0f, 50.0f, 10.0f, 0.0f };

	buttons.reserve(200);
	for (int i = 0; i < baseButtons.size(); i++)
	{
		Button* b = addButton(glm::vec2(0.0f, i * 8.0f), glm::vec2(50.0f, 6.0f), baseButtons[i], colors[i]);
		b->bools.set(INTERACTABLE); b->bools.set(RENDER_ENABLED); b->bools.set(MOVE_NEXT);
		b->openYMult = yMults[i];
	}

	float levelButtonSize = 6.9f;
	float lastX = 50.0f - levelButtonSize;
	float firtsX = levelButtonSize;

	int numX = 6;
	float yMult = (float)lastX / (numX - 1);
	bool done = false;
	int y;
	Button* playButton = &buttons[0];
	for (y = 0; !done; y++)
	{
		for (int x = 0; x < numX; x++)
		{
			int level = x + y * numX + 1;

			if (level >= 35)
			{
				done = true;
				break;
			}

			char c[3] = {};
			c[1] = '0' + level % 10;
			c[0] = '0' + level / 10;

			if (c[0] == '0')
			{
				c[0] = c[1];
				c[1] = '\0';
			}

			float blueAmount = (x + 1.0f) / numX;
			float redAmount = (y + 1.0f) / 6.0f;
			Button* b = addButton(glm::vec2(-25.0f + levelButtonSize * 0.5f + lastX * ((float)x / (numX - 1)),
				-y * yMult), glm::vec2(levelButtonSize), c, glm::vec4(redAmount, 0.5f, blueAmount, 0.2f));
			b->bools.set(INTERACTABLE);
			b->alphaMult = 0.0;
			playButton->dependentButtons.push_back(b);
		}
	}

	playButton->openYMult = yMult * y;
	playButton->openYAdder = 0.05f;

	std::vector<std::string> editorButtons = { "NEW", "LOAD", "COMMUNITY" };

	Button* editorButton = &buttons[1];
	for (int i = 0; i < editorButtons.size(); i++)
	{
		Button* b = addButton(glm::vec2(0.0f, -i * 8.0f), glm::vec2(50.0f, 6.0f), editorButtons[i], glm::vec4(0.0, 0.5, 1.0, 0.2));
		b->bools.set(INTERACTABLE);
		b->alphaMult = 0.0;
		editorButton->dependentButtons.push_back(b);
	}

	editorButton->openYMult = 24.0f;
	editorButton->openYAdder = 0.5f;

	Button* optionsButton = &buttons[2];

	std::vector<std::string> optionsButtons = { "GENERAL SOUND", "MUSIC", "SOUND EFFECTS" };

	soundButton = &buttons[buttons.size() - 1] + 3;
	for (int i = 0; i < optionsButtons.size(); i++)
	{
		Button* b = addButton(glm::vec2(0.0f, -i * 6), glm::vec2(50.0f, 4.0f), optionsButtons[i], glm::vec4(0.6, 0.6, 1.0, 1.0f));
		b->bools.set(TEXT_LEFT);
		b->alphaMult = 0.0;
		optionsButton->dependentButtons.push_back(b);

		b = addButton(glm::vec2(10.0f, -i * 6 - 2.0f), glm::vec2(25.0f, 0.0f), "", glm::vec4(0.0f), glm::vec2(1.25f));
		b->bools.set(INTERACTABLE);
		b->alphaMult = 0.0; b->baseAlpha = 1.0f;
		optionsButton->dependentButtons.push_back(b);

		b = addButton(glm::vec2(17.5f, -i * 6 - 2.0f), glm::vec2(0.0f, 0.0f), "", glm::vec4(1.0f), glm::vec2(3.0f));
		b->bools.set(INTERACTABLE); b->type = BUTTON_CIRCLE;
		b->alphaMult = 0.0; b->baseAlpha = 0.3f;
		optionsButton->dependentButtons.push_back(b);
	}

	glm::vec4 clrs[6] = { glm::vec4(1.0f, 1.0f, 0.0f, 1.0f),
		glm::vec4(0.0f, 0.5f, 1.0f, 1.0f), glm::vec4(1.0f, 1.0f, 0.0f, 1.0f), glm::vec4(0.0f, 0.5f, 1.0f, 1.0f),
		glm::vec4(0.0f, 0.5f, 1.0f, 1.0f), glm::vec4(0.0f, 0.5f, 1.0f, 1.0f) };

	SDL_Scancode scanCodes[6] = { SDL_SCANCODE_Q, SDL_SCANCODE_W, SDL_SCANCODE_E, SDL_SCANCODE_A, SDL_SCANCODE_S, SDL_SCANCODE_D };

	qButton = &buttons[buttons.size() - 1] + 3;
	float bSize = 46.0f / 3;
	for (int y = 0; y < 2; y++)
	{
		for (int x = 0; x < 3; x++)
		{
			Button* b = addButton(glm::vec2(-25.0f + bSize * 0.5f + x * (bSize + 2.0f), -y * 14.0f - 18.0f),
				glm::vec2(bSize - 0.5f, 11.5f), "", clrs[y * 3 + x], glm::vec2(1.0f, 1.5f));
			b->alphaMult = 0.0; b->baseAlpha = 0.1f;
			optionsButton->dependentButtons.push_back(b);

			glm::vec2 pos = glm::vec2(b->data.posSize.x, b->data.posSize.y + 0.75f);
			b = addButton(pos, glm::vec2(0.0f, 5.0f), keys[y * 3 + x], glm::vec4(0.0f));
			b->alphaMult = 0.0; b->baseAlpha = 0.0f;
			optionsButton->dependentButtons.push_back(b);

			const char* text = SDL_GetScancodeName(scanCodes[y * 3 + x]);

			pos.y -= 5.5f;
			b = addButton(pos, glm::vec2(bSize * 0.8f, 5.75f), text, glm::vec4(1.0f), glm::vec2(1.5f));
			b->alphaMult = 0.0; b->baseAlpha = 0.0f;
			b->bools.set(INTERACTABLE);
			optionsButton->dependentButtons.push_back(b);
		}
	}

	optionsButton->openYMult = 46.0f;
	optionsButton->openYAdder = 0.15f;
}

void Menu::addTextToButton(Button& b, std::string text)
{
	float texSize = b.data.posSize.w * 0.8f;
	float camY = ol->cam.defaultYSides.y * 2.0f;
	float screenY = 1000.0f;
	int reqTexSize = (int)(2.0f * texSize * screenY / camY);

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
	b.data.data = glm::vec4(data, 0.0f, 0.0f);
	b.generateVertices();

	addTextToButton(b, text);

	b.baseX = pos.x;
	b.baseY = pos.y;

	buttons.push_back(b);

	return &buttons.back();
}

void Menu::update()
{
	glm::vec2 mousePos = ol->cam.getMouseCoords();
	double timeNow = Game::object->gameTime;
	double dt = Game::object->deltaTime;

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
	}
	else
	{
		val01 = 0.0f;
	}

	float yNow;
	if (upCamTime > 0.0)
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
					dependentButton->alphaMult += Game::object->deltaTime * 4.0;
				}
				else
				{
					dependentButton->alphaMult -= Game::object->deltaTime * button.openYMult * 0.2;
				}

				dependentButton->alphaMult = dependentButton->alphaMult;
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

			button.activeAmount = button.activeAmount - Game::object->deltaTime * 2.0;

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
			ol->cd->renderDatas.push_back(button.text);

		if (button.bools[INTERACTABLE])
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
			if ((hoverButton + 1) < buttons.data() + buttons.size() && (hoverButton + 1)->type == BUTTON_CIRCLE)
			{
				clickButton = hoverButton + 1;
			}
			else
			{
				clickButton = hoverButton;
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
			float lastPos = clickButton->data.posSize.x;
			clickButton->data.posSize.x = std::clamp(mousePos.x, minX, maxX);
			
			if (abs(clickButton->data.posSize.x - lastPos) > 0.1f)
			{
				float ratio = (clickButton->data.posSize.x - minX) / (maxX - minX);
				int buttonIndex = (clickButton - soundButton) / 3;
				switch (buttonIndex)
				{
				case 0: Game::object->sounder->generalVolume = ratio; break;
				case 1: Game::object->sounder->musicVolume = ratio; break;
				case 2: Game::object->sounder->effectVolume = ratio; break;
				default: break;
				}

				Game::object->saveGameData();
			}
		}
	}
	else if (clickButton)
	{
		clickButton->text.color = glm::vec4(1.0f);
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

				Game::object->saveGameData();

				keyAssignmentButton = nullptr;

				break;
			}
		}
	}

	lastMousePos = mousePos;
	lastMouseTime = td->wh->lastMouseMovementTime;

	td->wh->cam->zoom = sin(td->gameTime * 0.5f) * 0.02 + cos(td->gameTime * 0.31f) * 0.02 + 0.5;
	td->wh->cam->updateOrtho();
}

void Menu::buttonPressed(Button* button)
{
	if (button->type == BUTTON_CIRCLE) return;
	if ((button + 1) < buttons.data() + buttons.size() && (button + 1)->type == BUTTON_CIRCLE) return;

	if (button >= qButton && button < qButton + 18)
	{
		keyAssignmentButton = button;
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

	if (button->openYMult == 0.0 || activeButton == button)
	{
		upCamTime = -td->gameTime;
		activeButton = nullptr;
	}
	else
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

	if (button->strText == "EXIT")
	{
		Game::object->shouldQuit = true;
	}
	else if (button->strText == "NEW")
	{
		Game::object->gameStater = &Game::LevelerState;
	}
}

void Menu::draw(VkCommandBuffer cmdBuff, void* ptr)
{
	double timeNow = Game::object->gameTime;

	Menu* menu = (Menu*)ptr;

	Leveler::renderParticles(cmdBuff, nullptr);

	ThinDrawer* td = ThinDrawer::object;
	Overlayer* ol = Leveler::object->ol;

	const uint32_t camOffset = ShaderBase::alignedSharedSize * (td->lastSwapChainImageIndex + 8);

	glm::vec2 mousePos = ol->cam.getMouseCoords();

	for (Button& button : menu->buttons)
	{
		if (!button.bools[RENDER_ENABLED]) continue;

		vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, td->menuButton->pipeline);

		vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
			td->menuButton->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

		vkCmdPushConstants(cmdBuff, td->menuButton->pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_menuButton), &button.data);

		vkCmdDraw(cmdBuff, 6, 1, 0, 0);
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
		float right = data.posSize.x + size.x;
		float top = data.posSize.y + size.y;
		float left = data.posSize.x - size.x;
		float bottom = data.posSize.y - size.y;
		if (pos.x >= left && pos.x <= right && pos.y >= bottom && pos.y <= top)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else if (type == BUTTON_CIRCLE)
	{
		float dist = glm::distance(pos, glm::vec2(data.posSize.x, data.posSize.y));
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