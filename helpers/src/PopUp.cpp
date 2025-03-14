#include "PopUp.h"
#include "ThinDrawer.h"
#include "Game.h"
#include "Overlayer.h"
#include "Menu.h"
#include <filesystem>
#include <thread>

static const std::string bans = "/\\*:?\"<>|";
static const std::string spacedBans = " / \\ * : ? \" < > |";
static double lastStatusTextShowTime = 0.0;

PopUp::PopUp()
{
	
}

void PopUp::construct()
{
	active = false;

	renderFunctions["saveRender"] = &PopUp::saveRender;
	initFunctions["saveRender"] = &PopUp::initSave;

	renderFunctions["loadRender"] = &PopUp::loadRender;
	initFunctions["loadRender"] = &PopUp::initLoad;

	renderFunctions["languageRender"] = &PopUp::languageRender;
	initFunctions["languageRender"] = &PopUp::initLanguage;

	renderFunctions["gpuRender"] = &PopUp::gpuRender;
	initFunctions["gpuRender"] = &PopUp::initGpu;

	renderFunctions["darkness"] = &PopUp::darknessRender;
	initFunctions["darkness"] = &PopUp::initDarkness;

	renderFunctions["creditsRender"] = &PopUp::creditsRender;
	initFunctions["creditsRender"] = &PopUp::initCredits;

	sliderHitSound = Sounder::object->loadSound("sliderHit.wav");
}

void PopUp::init(s_menuButton initialButton, std::string popName, double animTime)
{
	if (initFunctions.find(popName) == initFunctions.end() || (active && !closeOnEnd))
	{
		return;
	}

	this->initialButton = initialButton;
	this->startTime = ThinDrawer::object->gameTime;
	this->animationTime = animTime;
	this->extraTextDisplayAmount = 2.0;
	this->extraTextDisplayTime = -5.0;
	this->closeOnEnd = false;
	this->noAction = false;
	this->active = true;

	ThinDrawer::object->eraseFromRenderFunctions(ThinDrawer::object->lateDrawFuncs, this);
	ThinDrawer::object->lateDrawFuncs.push_back({ renderFunctions[popName], this});

	(this->*initFunctions[popName])();
}

void PopUp::initSave()
{
	SDL_StartTextInput(ThinDrawer::object->wh->window);

	renderTexts.resize(1); renderTexts[0] = "";
	textSizes.clear();

	finalButton = {
		glm::vec4(0.0f, 0.0f, 60.0f, 15.0f), // posSize
		glm::vec4(0.05, 0.05, 0.05, 1.0), // clr
		glm::vec4(5.0f, 5.25f, 1.1f, 1.0f) }; // data
}

void setClose(PopUp* p, s_menuButton* currentButton=nullptr)
{
	if (p->noAction || p->closeOnEnd || !p->active) return;

	if (currentButton)
	{
		p->initialButton = *currentButton;
	}
	else
	{
		p->initialButton = p->finalButton;
	}

	p->finalButton.posSize.z = -5.0f;
	p->finalButton.posSize.w = -5.0f;
	p->finalButton.colour.w = 0.0f;

	p->startTime = ThinDrawer::object->gameTime;
	p->animationTime = 0.5f;

	p->closeOnEnd = true;
	p->active = false;
}

void saveDialogCallBack(void* userdata, const char* const* filelist, int filter)
{
	PopUp* p = (PopUp*)userdata;

	p->noAction = false;
	if (!p->active) return;

	if (!filelist || !(*filelist)) return;

	if (strlen(*filelist) > 0)
	{
		std::string copyString = *filelist;

		Game::object->taskRegistryMutex.lock();

		Game::object->singleTaskRegistry.push_back([p, copyString]
		{
			int res = Leveler::object->saveLevel(copyString, true);
			if (res == 0)
			{
				setClose(p);
			}
		});

		Game::object->taskRegistryMutex.unlock();
	}
}

void openLevel(std::string fileName, PopUp* pop, bool externalOpen=false)
{
	setClose(pop);

	pop->endFunction = [pop, fileName, externalOpen]
	{
		Game::object->singleTaskRegistry.push_back([pop, fileName, externalOpen]
		{
			int res = Leveler::object->openLevel(fileName, false, externalOpen);
			if (res == 0)
			{
				s_menuButton initialButton = {
				glm::vec4(0.0f, 0.0f, 300.0f, 300.0f), // posSize
				glm::vec4(0.0f, 0.0f, 0.0f, 1.0), // clr
				glm::vec4(5.0f, 5.5f, 1.5f, 0.1f) }; // data
				pop->init(initialButton, "darkness", 0.5f);
				pop->endFunction.reset();
			}
		});

		if (Game::object->gameStater == &Game::MenuState)
		{
			Game::object->gameStater = &Game::LevelerState;
		}
	};
}

void openDialogCallBack(void* userdata, const char* const* filelist, int filter)
{
	PopUp* p = (PopUp*)userdata;

	p->noAction = false;
	if (!p->active) return;

	if (!filelist || !(*filelist)) return;

	if (strlen(*filelist) > 0)
	{
		std::string fileName(*filelist);
		openLevel(fileName, p, true);
	}
}

SDL_DialogFileFilter filters[] =
{
	{ "Game Level Data", "pld" },
	{ "All Files", "*" }
};

void PopUp::saveRender(VkCommandBuffer cmdBuff, void* ptr)
{
	static char banChar = '\0';
	static double hoverTimes[2] = { 0.0, 0.0 };

	ThinDrawer* td = ThinDrawer::object;
	PopUp* p = (PopUp*)ptr;

	const uint32_t camOffset = ShaderBase::alignedSharedSize * (td->frameIndex + 8);

	vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, td->menuButton->pipeline);

	vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
		td->menuButton->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

	float td01 = std::min((td->gameTime - p->startTime) / p->animationTime, 1.0);
	td01 = (sin(td01 * glm::pi<float>() - glm::pi<float>() * 0.5f) + 1.0f) * 0.5f;

	if (td01 > 0.999f && p->closeOnEnd)
	{
		SDL_StopTextInput(ThinDrawer::object->wh->window);

		Game::object->singleTaskRegistry.push_back([p]
		{
			ThinDrawer::object->eraseFromRenderFunctions(ThinDrawer::object->lateDrawFuncs, p);
			p->active = false;
		});
	}

	s_menuButton currentButtonData;
	currentButtonData.colour = p->initialButton.colour + (p->finalButton.colour - p->initialButton.colour) * td01;
	currentButtonData.data = p->initialButton.data + (p->finalButton.data - p->initialButton.data) * td01;
	currentButtonData.posSize = p->initialButton.posSize + (p->finalButton.posSize - p->initialButton.posSize) * td01;

	float sizedt = currentButtonData.posSize.w / 15.0f;

	s_menuButton bgView;
	bgView.colour = glm::vec4(0.0f, 0.0f, 0.0f, sizedt);
	bgView.posSize = glm::vec4(0.0f, 0.0f, Leveler::object->ol->cam.xSides.y * 2.0f, Leveler::object->ol->cam.ySides.y * 2.0f);
	bgView.data = glm::vec4(0.0f, 0.0f, 1.1f, 0.25f);

	vkCmdPushConstants(cmdBuff, td->menuButton->pipelineLayout,
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_menuButton), &bgView);

	vkCmdDraw(cmdBuff, 6, 1, 0, 0);

	vkCmdPushConstants(cmdBuff, td->menuButton->pipelineLayout,
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_menuButton), &currentButtonData);

	vkCmdDraw(cmdBuff, 6, 1, 0, 0);

	if (td->wh->keyData[SDL_SCANCODE_RETURN] == 2 && p->renderTexts[0].size() != 0 && !p->noAction)
	{
		int res = Leveler::object->saveLevel(IOHelper::assetsFolder + "levelfiles/" + p->renderTexts[0]);

		if (res == 0) setClose(p, &currentButtonData);
	}

	if (td->wh->repeatKeys[SDL_SCANCODE_BACKSPACE] || td->wh->keyData[SDL_SCANCODE_BACKSPACE] == 2)
	{
		if (p->renderTexts[0].size() > 0 && !p->noAction)
		{
			p->renderTexts[0].resize(p->renderTexts[0].size() - p->textSizes.back());
			p->textSizes.pop_back();
		}
	}

	if ((td->wh->keyData[SDL_SCANCODE_ESCAPE] == 2 || 
		!currentButtonData.hoverCheck(Leveler::object->ol->cam.getMouseCoords()) && td->wh->mouseData[2] == 2)
		&& !p->noAction)
	{
		setClose(p, &currentButtonData);

		if (Leveler::object->ol->overlayOverlay == SAVE_LEVEL)
		{
			Leveler::object->ol->overlayOverlay = LEVELER;
		}
	}

	if (td->wh->text.size() > 0 && p->textSizes.size() < 32 && (td->wh->text != " " || p->textSizes.size() > 0))
	{
		for (char c : bans)
		{
			for (char tc : td->wh->text)
			{
				if (c == tc)
				{
					p->extraTextDisplayTime = td->gameTime;
					banChar = c;
					goto noAdd;
				}
			}
		}

		p->renderTexts[0] += td->wh->text;
		p->textSizes.push_back(td->wh->text.size());
	}

noAdd:

	if (td->gameTime - p->extraTextDisplayTime < p->extraTextDisplayAmount  && !p->closeOnEnd)
	{
		s_lineRenderData lrd;
		lrd.charDatas = Leveler::object->ol->pops["leveler_options"].texts[3];

		float x = Leveler::object->ol->cd->calcSize(lrd.charDatas);
		float x2;
		auto banRenderData = Leveler::object->ol->cd->getRenderData(spacedBans +
			"              " + banChar, Leveler::object->ol->popHeight, x2);

		float totalSize = x + x2;

		lrd.color = glm::vec4(0.6, 0.0, 0.0, 1.0f - (td->gameTime - p->extraTextDisplayTime) / p->extraTextDisplayAmount);
		lrd.posScale = glm::vec4(currentButtonData.posSize.x - totalSize * 1.25f,
			currentButtonData.posSize.y + 7.5f, 2.5f, 0.0);

		Leveler::object->ol->cd->renderDatas.push_back(lrd);

		lrd.posScale.x += x * 2.5f;
		lrd.charDatas = banRenderData;
		Leveler::object->ol->cd->renderDatas.push_back(lrd);
	}

	const float renderSize = 5.0f;
	float x;

	if (p->renderTexts[0].size() > 0)
	{
		auto renderData = Leveler::object->ol->cd->getRenderData(p->renderTexts[0], Leveler::object->ol->popHeight * 2, x);

		s_lineRenderData lrd;
		lrd.charDatas = renderData;
		lrd.color = glm::vec4(0.0, 0.0, 0.0, sizedt);
		lrd.posScale = glm::vec4(currentButtonData.posSize.x - x * renderSize * 0.5f,
			currentButtonData.posSize.y + 2.0f, renderSize, x);

		Leveler::object->ol->cd->renderDatas.push_back(lrd);
	}
	else
	{
		int lang = Leveler::object->ol->cd->selectedLanguageIndex;
		auto renderData = Leveler::object->ol->cd->getRenderData(Game::object->menu->buttonTexts["<enterName>"][lang],
			Leveler::object->ol->popHeight * 2, x);

		s_lineRenderData lrd;
		lrd.charDatas = renderData;
		lrd.color = glm::vec4(0.0, 0.0, 0.0, td01 * 0.5f);
		lrd.posScale = glm::vec4(currentButtonData.posSize.x - x * renderSize * 0.5f,
			currentButtonData.posSize.y + 2.0f, renderSize, x);

		Leveler::object->ol->cd->renderDatas.push_back(lrd);
	}

	if (x * renderSize > p->finalButton.posSize.z - 20.0f && !p->closeOnEnd)
	{
		p->initialButton = currentButtonData;
		p->finalButton = currentButtonData;
		p->finalButton.posSize.z = x * renderSize + 20.0f;
		p->startTime = ThinDrawer::object->gameTime;
		p->animationTime = 0.05f;
	}
	else if (x * renderSize < p->finalButton.posSize.z - 20.0f && x * renderSize > 40.0f && !p->closeOnEnd)
	{
		p->initialButton = currentButtonData;
		p->finalButton = currentButtonData;
		p->finalButton.posSize.z = x * renderSize + 20.0f;
		p->startTime = ThinDrawer::object->gameTime;
		p->animationTime = 0.05f;
	}

	currentButtonData.posSize.y += 2.0f;
	currentButtonData.posSize.z -= 10.0f;
	currentButtonData.posSize.w *= 0.15f;
	currentButtonData.colour = glm::vec4(0.5f, 0.5f, 0.5f, currentButtonData.colour.w);
	currentButtonData.data.z = 1.3f;

	vkCmdPushConstants(cmdBuff, td->menuButton->pipelineLayout,
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_menuButton), &currentButtonData);

	vkCmdDraw(cmdBuff, 6, 1, 0, 0);

	currentButtonData.posSize.x += 12.0f;
	currentButtonData.posSize.y -= 8.0f;
	currentButtonData.posSize.z -= 30.0f;
	currentButtonData.posSize.z = std::min(currentButtonData.posSize.z, 20.0f);
	currentButtonData.posSize.w += 1.5f;
	currentButtonData.data = glm::vec4(1.0f, 1.25f, 1.2f, 1.0f);

	if (p->textSizes.size() > 0)
	{
		bool hovered = currentButtonData.hoverCheck(Leveler::object->ol->cam.getMouseCoords());

		if (hovered)
		{
			hoverTimes[0] = td->gameTime;
			if (td->wh->mouseData[2] == 2 && p->active)
			{
				int res = Leveler::object->saveLevel(IOHelper::assetsFolder + "levelfiles/" + p->renderTexts[0]);
				
				if (res == 0) setClose(p);
			}
		}
	}

	float baseW = currentButtonData.colour.w;

	float hover01 = std::clamp((td->gameTime - hoverTimes[0]) * 4.0f, 0.0, 1.0);
	currentButtonData.colour = glm::vec4(0.0f, 1.0f, 0.0f, std::clamp(baseW * 0.7f - hover01 * 0.3f, 0.0f, 1.0f));
	currentButtonData.data.y = 2.25f - hover01;

	vkCmdPushConstants(cmdBuff, td->menuButton->pipelineLayout,
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_menuButton), &currentButtonData);

	vkCmdDraw(cmdBuff, 6, 1, 0, 0);

	s_lineRenderData lrd;
	lrd.charDatas = Leveler::object->ol->pops["leveler_options"].texts[1];
	lrd.color = glm::vec4(0.0f, 0.0f, 0.0f, currentButtonData.colour.w * 2.0f);

	x = Leveler::object->ol->cd->calcSize(lrd.charDatas);

	lrd.posScale = glm::vec4(currentButtonData.posSize.x - x * 1.25f, currentButtonData.posSize.y, 2.5f, 0.0f);
	Leveler::object->ol->cd->renderDatas.push_back(lrd);

	currentButtonData.posSize.x -= 24.0f;

	if (currentButtonData.hoverCheck(Leveler::object->ol->cam.getMouseCoords()))
	{
		hoverTimes[1] = td->gameTime;
		if (td->wh->mouseData[2] == 2 && p->active)
		{
			p->noAction = true;
			SDL_ShowSaveFileDialog(saveDialogCallBack, p, td->wh->window, filters, std::size(filters), nullptr);
		}
	}

	hover01 = std::clamp((td->gameTime - hoverTimes[1]) * 4.0f, 0.0, 1.0);
	currentButtonData.colour = glm::vec4(0.6f, 0.0f, 0.1f, std::clamp(baseW * 0.7f - hover01 * 0.3f, 0.0f, 1.0f));
	currentButtonData.data.y = 2.25f - hover01;

	vkCmdPushConstants(cmdBuff, td->menuButton->pipelineLayout,
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_menuButton), &currentButtonData);

	vkCmdDraw(cmdBuff, 6, 1, 0, 0);

	lrd.charDatas = Leveler::object->ol->pops["leveler_options"].texts[4];
	lrd.color = glm::vec4(0.0f, 0.0f, 0.0f, currentButtonData.colour.w * 2.0f);

	x = Leveler::object->ol->cd->calcSize(lrd.charDatas);

	lrd.posScale = glm::vec4(currentButtonData.posSize.x - x * 1.25f, currentButtonData.posSize.y, 2.5f, 0.0f);
	Leveler::object->ol->cd->renderDatas.push_back(lrd);

	if (p->closeOnEnd)
	{
		for (s_lineRenderData& lrd : Leveler::object->ol->cd->renderDatas)
		{
			float m = std::max(1.0f - td01 * 3.0f, 0.0f);
			lrd.color.w *= m;
		}
	}

	CharDrawer::draw(cmdBuff, Leveler::object->ol->cd);
}

static std::vector<levelSelectData> levelSelects;
static const glm::vec2 size = glm::vec2(115.0f, 6.0f);
static s_lineRenderData loadRenderData;
static float scrollSpeed = 0.0f;

void loadbgThread()
{
	std::vector<std::string> levelFiles;
	std::string path = IOHelper::assetsFolder + "levelfiles/";
	for (const auto& entry : std::filesystem::directory_iterator(path))
	{
		auto filename = entry.path().filename();
		std::u8string u8str = filename.u8string();
		const char8_t* cstr = u8str.c_str();
		std::string str = std::string((char*)cstr);
		if (str.size() > 4 && str.substr(str.size() - 4) == ".pld")
		{			
			str = str.substr(0, str.size() - 4);

			levelFiles.push_back(str);
		}
	}

	for (int i = 0; i < 0; i++)
	{
		std::string randStr = std::to_string(i);

		//for (int i = 0; i < 10; i++)
		//{
		//	int randIndex = rand() % 26;
		//	randStr += 'a' + randIndex;
		//}

		levelFiles.push_back(randStr);
	}

	levelSelects.resize(levelFiles.size());

	PopUp* p = &Game::object->pop;
	while (!p->closeOnEnd)
	{
		loadRenderData.color = glm::vec4(1.0f);
		loadRenderData.posScale = glm::vec4(0.0f, 0.0f, 5.0f, 0.0f);
		std::string loadingString = ".";
		for (int i = 0; i < ((int)(ThinDrawer::object->gameTime * 5.0f)) % 5; i++)
		{
			loadingString += ".";
		}
		
		Leveler::object->ol->cd->loadMutex.lock();

		loadRenderData.charDatas = Leveler::object->ol->cd->getRenderData(loadingString, Leveler::object->ol->popHeight * 2, loadRenderData.posScale.z);

		Leveler::object->ol->cd->loadMutex.unlock();

		uint32_t renderBegin = (int)(p->scrollAmount / (size.y + 1.0f));
		uint32_t renderEnd = std::min(renderBegin + 10, (uint32_t)levelSelects.size());

		for (int i = renderBegin; i < renderEnd; i++)
		{
			LevelSelectData& lsd = levelSelects[i];
			if (lsd.status == lLoadEnum::L_NONE)
			{
				lsd.status = L_LOADING;
				lsd.textData.posScale = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);

				lsd.text = levelFiles[i];
				lsd.textData.charDatas = Leveler::object->ol->cd->getRenderData(lsd.text, Leveler::object->ol->popHeight * 2, lsd.textData.posScale.w);

				lsd.textData.color = glm::vec4(1.0f);
				lsd.status = L_LOADED;
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

void PopUp::initLoad()
{
	// filter
	// SDL_StartTextInput(ThinDrawer::object->wh->window);

	scrollAmount = 0.0f;
	targetScroll = 0.0f;
	scrollClickIndex = -1;
	clickOffset = 0.0f;
	totalSize = 100.0f; // change
	scrollSpeed = 0.0f;

	renderTexts.resize(1); renderTexts[0] = "";
	textSizes.clear();

	endFunction.reset();

	finalButton = {
		glm::vec4(0.0f, 0.0f, 120.0f, 70.0f), // posSize
		glm::vec4(0.04, 0.04, 0.05, 1.0), // clr
		glm::vec4(5.0f, 5.25f, 1.1f, 1.0f) }; // data

	levelSelects.clear();

	std::thread(loadbgThread).detach();
}

void PopUp::renderAt(VkCommandBuffer cmdBuff, glm::vec4 posSize, glm::vec2 pos, levelSelectData data)
{
	float maxY = +posSize.w * 0.5f - 8.0f + posSize.y;
	float minY = -posSize.w * 0.5f + posSize.y;

	float maxX = +posSize.z * 0.5f - 3.0f + posSize.x;
	float minX = -posSize.z * 0.5f + 3.0f + posSize.x;

	float sizedt = posSize.z / 120.0f;

	glm::vec2 newSize = sizedt * size;

	s_menuButton buttonData;
	buttonData.posSize = glm::vec4(pos, newSize - 1.0f);
	buttonData.colour = glm::vec4(0.3f);
	buttonData.data = glm::vec4(1.0f, 1.25f, 1.0f, 1.0f);

	float overlapAmount = pos.y + newSize.y * 0.5f - maxY;
	float overlapPerct = 0.0f;
	if (overlapAmount > 0.0f)
	{
		buttonData.posSize.w -= overlapAmount;
		buttonData.posSize.y -= overlapAmount * 0.5f;
		overlapPerct = overlapAmount / newSize.y;
	}
	else if ((overlapAmount = minY - (pos.y - newSize.y * 0.5f)) > 0.0f)
	{
		buttonData.posSize.w -= overlapAmount;
		buttonData.posSize.y += overlapAmount * 0.5f;
		overlapPerct = overlapAmount / newSize.y;
	}

	if (buttonData.posSize.w <= -1.0f) return;

	if (buttonData.hoverCheck(Leveler::object->ol->cam.getMouseCoords()))
	{
		buttonData.colour.w = 1.0f;

		if (ThinDrawer::object->wh->mouseData[2] == 2 && active && data.status == lLoadEnum::L_LOADED)
		{
			std::string fileName = IOHelper::assetsFolder + "levelfiles/" + data.text + ".pld";
			openLevel(fileName, this);
		}
	}

	vkCmdPushConstants(cmdBuff, ThinDrawer::object->menuButton->pipelineLayout,
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_menuButton), &buttonData);

	vkCmdDraw(cmdBuff, 6, 1, 0, 0);

	float textX = buttonData.posSize.x - buttonData.posSize.z * 0.5f + 2.0f;

	if (data.status == lLoadEnum::L_LOADED)
	{
		float xSize = data.textData.posScale.w;
		float xMult = (1.0f - overlapPerct) * sizedt * std::min(12.5f / xSize, 1.0f);
		data.textData.posScale = glm::vec4(textX, buttonData.posSize.y, 5.0f * xMult, data.textData.posScale.w);
		data.textData.color.w = (0.5f - overlapPerct) * sizedt * 2.0f;
		Leveler::object->ol->cd->renderDatas.push_back(data.textData);
	}
	else
	{
		loadRenderData.posScale = glm::vec4(textX, buttonData.posSize.y, (5.0f - overlapPerct * 5.0f) * sizedt, 0.0f);
		loadRenderData.color.w = (0.5f - overlapPerct) * sizedt * 2.0f;
		Leveler::object->ol->cd->renderDatas.push_back(loadRenderData);
	}
}

void PopUp::loadRender(VkCommandBuffer cmdBuff, void* ptr)
{
	ThinDrawer* td = ThinDrawer::object;
	PopUp* p = (PopUp*)ptr;

	const uint32_t camOffset = ShaderBase::alignedSharedSize * (td->frameIndex + 8);

	vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, td->menuButton->pipeline);

	vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
		td->menuButton->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

	float td01 = std::min((td->gameTime - p->startTime) / p->animationTime, 1.0);
	td01 = (sin(td01 * glm::pi<float>() - glm::pi<float>() * 0.5f) + 1.0f) * 0.5f;

	s_menuButton currentButtonData;
	currentButtonData.colour = p->initialButton.colour + (p->finalButton.colour - p->initialButton.colour) * td01;
	currentButtonData.data = p->initialButton.data + (p->finalButton.data - p->initialButton.data) * td01;
	currentButtonData.posSize = p->initialButton.posSize + (p->finalButton.posSize - p->initialButton.posSize) * td01;

	float sizedt = currentButtonData.posSize.w / 70.0f;

	s_menuButton bgView;
	bgView.colour = glm::vec4(0.0f, 0.0f, 0.0f, sizedt);
	bgView.posSize = glm::vec4(0.0f, 0.0f, Leveler::object->ol->cam.xSides.y * 2.0f, Leveler::object->ol->cam.ySides.y * 2.0f);
	bgView.data = glm::vec4(0.0f, 0.0f, 1.1f, 0.1f);

	if (p->endFunction.has_value())
	{
		bgView.colour.w = 1.0;
		bgView.data.z = 1.6f - sizedt * 0.5f;
	}

	vkCmdPushConstants(cmdBuff, td->menuButton->pipelineLayout,
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_menuButton), &bgView);

	vkCmdDraw(cmdBuff, 6, 1, 0, 0);

	glm::vec2 pos = glm::vec2(currentButtonData.posSize);

	vkCmdPushConstants(cmdBuff, td->menuButton->pipelineLayout,
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_menuButton), &currentButtonData);

	vkCmdDraw(cmdBuff, 6, 1, 0, 0);

	if (td01 > 0.999f && p->closeOnEnd)
	{
		if (p->endFunction.has_value())
		{
			ThinDrawer::object->addPopText("loading", glm::vec2(0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));
		}
		
		Game::object->singleTaskRegistry2.push_back([p]
		{
			if (p->endFunction.has_value())
			{
				p->endFunction.value()();
				p->endFunction.reset();
			}

			ThinDrawer::object->eraseFromRenderFunctions(ThinDrawer::object->lateDrawFuncs, p);
			p->active = false;
		});
	}

	if (td->wh->keyData[SDL_SCANCODE_ESCAPE] == 2 ||
		!currentButtonData.hoverCheck(Leveler::object->ol->cam.getMouseCoords()) && td->wh->mouseData[2] == 2)
	{
		setClose(p, &currentButtonData);

		if (Leveler::object->ol->overlayOverlay == LOAD_LEVEL)
		{
			Leveler::object->ol->overlayOverlay = LEVELER;
		}
	}

	float sizep1 = size.y + 1.0f;

	float maxScroll = std::max(levelSelects.size() * sizep1 - sizep1 * 7.0f, 0.0f);
	float leftX = currentButtonData.posSize.x - (currentButtonData.posSize.z + currentButtonData.data.y) * 0.5f;
	float rightX = currentButtonData.posSize.x + (currentButtonData.posSize.z + currentButtonData.data.y) * 0.5f;
	s_menuButton buttonData;

	if (maxScroll > 10.0f)
	{
		buttonData.posSize = glm::vec4(leftX + 2.5f, currentButtonData.posSize.y - 4.0f, 0.0f, 60.0f * sizedt);
		buttonData.colour = glm::vec4(0.4f * sizedt);
		buttonData.data = glm::vec4(0.25f, 0.5f, -1.0f, 1.0f);

		vkCmdPushConstants(cmdBuff, ThinDrawer::object->menuButton->pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_menuButton), &buttonData);

		vkCmdDraw(cmdBuff, 6, 1, 0, 0);

		buttonData.posSize.x = rightX - 2.5f;

		vkCmdPushConstants(cmdBuff, ThinDrawer::object->menuButton->pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_menuButton), &buttonData);

		vkCmdDraw(cmdBuff, 6, 1, 0, 0);

		const glm::vec2 sliderSize = glm::vec2(0.75f, std::clamp(75.0f - maxScroll, 8.0f, 60.0f)) * sizedt;

		if (td->wh->mouseData[2] == 0) p->scrollClickIndex = -1;

		buttonData.data.x = 0.75f;
		buttonData.data.y = 1.0f;
		buttonData.colour *= 2.0f;

		const float maxY = buttonData.posSize.y + (buttonData.posSize.w + buttonData.data.y - sliderSize.y - buttonData.data.y) * 0.5f;
		const float minY = buttonData.posSize.y - (buttonData.posSize.w + buttonData.data.y - sliderSize.y - buttonData.data.y) * 0.5f;
		const float diff = maxY - minY;

		if (p->scrollClickIndex != -1)
		{
			glm::vec2 mousePos = Leveler::object->ol->cam.getMouseCoords();

			const float top01 = std::clamp((maxY - mousePos.y - p->clickOffset) / diff, 0.0f, 1.0f);
			float newScroll = top01 * maxScroll;

			scrollSpeed = (newScroll - p->scrollAmount) * 0.2f + scrollSpeed * 0.8f;
			p->scrollAmount = newScroll;
		}
		else if (abs(scrollSpeed) > 0.01f)
		{
			float addScroll = scrollSpeed * td->deltaTime * 6.0f;
			scrollSpeed -= addScroll;
			p->scrollAmount += scrollSpeed;

			if (p->scrollAmount > maxScroll)
			{
				float maxDiff = p->scrollAmount - maxScroll;
				p->scrollAmount = maxScroll - maxDiff;
				scrollSpeed *= -1.0f;
				Sounder::object->playSound(p->sliderHitSound, std::clamp(40.0f * abs(scrollSpeed) / maxScroll, 0.1f, 5.0f));
			}
			else if (p->scrollAmount < 0.0f)
			{
				p->scrollAmount = -p->scrollAmount;
				scrollSpeed *= -1.0f;
				Sounder::object->playSound(p->sliderHitSound, std::clamp(40.0f * abs(scrollSpeed) / maxScroll, 0.1f, 5.0f));
			}

			p->targetScroll = p->scrollAmount;
		}

		if (diff > 1.0f)
		{
			const float renderY = std::clamp(maxY - (p->scrollAmount / maxScroll) * diff, minY, maxY);

			buttonData.posSize = glm::vec4(leftX + 2.5f, renderY, sliderSize.x, sliderSize.y);

			glm::vec2 mousePos = Leveler::object->ol->cam.getMouseCoords();
			if (buttonData.hoverCheck(mousePos) && p->scrollClickIndex == -1)
			{
				buttonData.data.y = 1.5f;
				if (td->wh->mouseData[2] == 2)
				{
					p->scrollClickIndex = 0;
					p->clickOffset = buttonData.posSize.y - mousePos.y;
				}
			}
			else
			{
				buttonData.data.y = 1.0f;
			}

			if (p->scrollClickIndex == 0)
			{
				buttonData.data.y = 2.0f;
			}

			vkCmdPushConstants(cmdBuff, ThinDrawer::object->menuButton->pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_menuButton), &buttonData);

			vkCmdDraw(cmdBuff, 6, 1, 0, 0);

			buttonData.posSize.x = rightX - 2.5f;

			if (buttonData.hoverCheck(mousePos) && p->scrollClickIndex == -1)
			{
				buttonData.data.y = 1.5f;
				if (td->wh->mouseData[2] == 2)
				{
					p->scrollClickIndex = 1;
					p->clickOffset = buttonData.posSize.y - mousePos.y;
				}
			}
			else
			{
				buttonData.data.y = 1.0f;
			}

			if (p->scrollClickIndex == 1)
			{
				buttonData.data.y = 2.0f;
			}

			vkCmdPushConstants(cmdBuff, ThinDrawer::object->menuButton->pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_menuButton), &buttonData);

			vkCmdDraw(cmdBuff, 6, 1, 0, 0);
		}

		if (p->scrollClickIndex == -1)
		{
			if (td->wh->mouseData[5] != 0)
			{
				scrollSpeed *= 1.0 - td->deltaTime;
			}

			p->targetScroll -= td->wh->mouseData[5] * 10.0;

			p->scrollAmount += (p->targetScroll - p->scrollAmount) * td->deltaTime * 10.0f;

			p->targetScroll = std::clamp(p->targetScroll, 0.0f, maxScroll);
			p->scrollAmount = std::max(p->scrollAmount, -5.0f);
		}
		else
		{
			p->targetScroll = p->scrollAmount;
		}
	}

	const float xSize = 110.0f / 4.0f;
	uint32_t renderBegin = (int)(p->scrollAmount / sizep1);
	uint32_t renderEnd = std::min(renderBegin + 10, (uint32_t)levelSelects.size());
	for (int i = renderBegin; i < renderEnd; i++)
	{
		p->renderAt(cmdBuff, currentButtonData.posSize,
			glm::vec2(currentButtonData.posSize.x, -i * sizep1 + 23.5f + p->scrollAmount + pos.y), levelSelects[i]);
	}

	buttonData.posSize = glm::vec4(-currentButtonData.posSize.z * 0.5f + 12.0f, currentButtonData.posSize.w * 0.5f - 3.0f, 18.0f * sizedt, 4.0f * sizedt);
	buttonData.posSize.x += currentButtonData.posSize.x;
	buttonData.posSize.y += currentButtonData.posSize.y;
	buttonData.colour = glm::vec4(0.2f * sizedt);
	buttonData.colour.w *= 3.0f;
	buttonData.colour.x *= 2.5f;
	buttonData.data = glm::vec4(1.0f, 1.25f, 1.0f, 1.0f);

	if (buttonData.hoverCheck(Leveler::object->ol->cam.getMouseCoords()))
	{
		buttonData.data.y = 2.0f;
		if (td->wh->mouseData[2] == 2 && p->active && sizedt > 0.99f)
		{
			p->noAction = true;
			SDL_ShowOpenFileDialog(openDialogCallBack, p, td->wh->window, filters, std::size(filters), nullptr, false);
		}
	}

	vkCmdPushConstants(cmdBuff, ThinDrawer::object->menuButton->pipelineLayout,
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_menuButton), &buttonData);

	vkCmdDraw(cmdBuff, 6, 1, 0, 0);

	s_lineRenderData lrd;
	lrd.charDatas = Leveler::object->ol->pops["leveler_options"].texts[5];
	float x = Leveler::object->ol->cd->calcSize(lrd.charDatas);
	
	lrd.color = glm::vec4(1.0f * sizedt);
	lrd.posScale = glm::vec4(buttonData.posSize.x - x * 1.25f * sizedt, buttonData.posSize.y, 2.5f * sizedt, 0.0f);

	Leveler::object->ol->cd->renderDatas.push_back(lrd);

	CharDrawer::draw(cmdBuff, Leveler::object->ol->cd);
}

static std::vector<s_lineRenderData> renderTextDatas;
static float finalSize = 70.0f;

void PopUp::initLanguage()
{
	finalSize = std::min(Leveler::object->ol->cd->allLanguages.size() * 7.0f + 3.0f, 70.0f);

	finalButton = {
		glm::vec4(0.0f, 0.0f, 50.0f, finalSize), // posSize
		glm::vec4(0.04, 0.04, 0.08, 1.0), // clr
		glm::vec4(5.0f, 5.5f, 1.1f, 1.0f) }; // data

	renderTextDatas.clear();

	for (auto& str : Leveler::object->ol->cd->allLanguages)
	{
		s_lineRenderData lrd;
		lrd.charDatas = Leveler::object->ol->cd->getRenderData(str, Leveler::object->ol->popHeight * 2, lrd.posScale.w);
		lrd.color = glm::vec4(1.0f);
		lrd.posScale = glm::vec4(-lrd.posScale.w * 2.5f, 0.0f, 5.0f, lrd.posScale.w);
		renderTextDatas.push_back(lrd);
	}
}

void PopUp::languageRender(VkCommandBuffer cmdBuff, void* ptr)
{
	ThinDrawer* td = ThinDrawer::object;
	PopUp* p = (PopUp*)ptr;

	const uint32_t camOffset = ShaderBase::alignedSharedSize * (td->frameIndex + 8);

	vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, td->menuButton->pipeline);

	vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
		td->menuButton->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

	float td01 = std::min((td->gameTime - p->startTime) / p->animationTime, 1.0);
	td01 = (sin(td01 * glm::pi<float>() - glm::pi<float>() * 0.5f) + 1.0f) * 0.5f;

	s_menuButton currentButtonData;
	currentButtonData.colour = p->initialButton.colour + (p->finalButton.colour - p->initialButton.colour) * td01;
	currentButtonData.data = p->initialButton.data + (p->finalButton.data - p->initialButton.data) * td01;
	currentButtonData.posSize = p->initialButton.posSize + (p->finalButton.posSize - p->initialButton.posSize) * td01;

	float size01 = currentButtonData.posSize.w / finalSize;

	if (Game::object->gameStater == &Game::MenuState)
	{
		s_menuButton bgView;
		bgView.colour = glm::vec4(0.0f, 0.0f, 0.0f, size01);
		bgView.posSize = glm::vec4(0.0f, 0.0f, Leveler::object->ol->cam.xSides.y * 2.0f, Leveler::object->ol->cam.ySides.y * 2.0f);
		bgView.data = glm::vec4(0.0f, 0.0f, 1.1f, 1.0f);

		vkCmdPushConstants(cmdBuff, td->menuButton->pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_menuButton), &bgView);

		vkCmdDraw(cmdBuff, 6, 1, 0, 0);
	}

	vkCmdPushConstants(cmdBuff, td->menuButton->pipelineLayout,
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_menuButton), &currentButtonData);

	vkCmdDraw(cmdBuff, 6, 1, 0, 0);

	if (td01 > 0.999f && p->closeOnEnd)
	{
		Game::object->singleTaskRegistry.push_back([p]
		{
			ThinDrawer::object->eraseFromRenderFunctions(ThinDrawer::object->lateDrawFuncs, p);
			p->active = false;
		});
	}

	if (td->wh->keyData[SDL_SCANCODE_ESCAPE] == 2 ||
		!currentButtonData.hoverCheck(Leveler::object->ol->cam.getMouseCoords()) && td->wh->mouseData[2] == 2)
	{
		setClose(p, &currentButtonData);
	}

	std::string topText = Game::object->menu->buttonTexts["<langset_" + std::to_string(Leveler::object->ol->cd->selectedLanguageIndex) + ">"][0];

	float topStart = currentButtonData.posSize.y + currentButtonData.posSize.w * 0.5f;

	s_lineRenderData lrd;
	lrd.charDatas = Leveler::object->ol->cd->getRenderData(topText, Leveler::object->ol->popHeight, lrd.posScale.w);
	lrd.color = glm::vec4(0.3f, 0.0f, 0.0f, size01);
	lrd.posScale = glm::vec4(-lrd.posScale.w * 1.25f * size01, topStart, 2.5f * size01, 0.0f);

	Leveler::object->ol->cd->renderDatas.push_back(lrd);

	topStart -= 6.0f;

	int i = 0;
	for (auto& lrd : renderTextDatas)
	{
		lrd.posScale.x = -lrd.posScale.w * 2.5f * size01;
		lrd.posScale.y = topStart;
		lrd.posScale.z = size01 * 5.0f;
		lrd.color.w = size01;
		topStart -= 7.0f * size01;

		Leveler::object->ol->cd->renderDatas.push_back(lrd);

		s_menuButton button;
		if (i == Leveler::object->ol->cd->selectedLanguageIndex)
		{
			button.colour = glm::vec4(0.7f, 0.0f, 0.2f, 0.5f);
		}
		else
		{
			button.colour = glm::vec4(0.7f, 0.3f, 0.05f, 0.5f);
		}

		button.data = glm::vec4(2.5f, 3.0f, 1.1f, 0.0f) * size01;
		button.data.w = 1.0;
		button.posSize = glm::vec4(0.0f, lrd.posScale.y, 47.5f * size01, 3.5f * size01);

		
		if (button.hoverCheck(Leveler::object->ol->cam.getMouseCoords()) && p->active)
		{
			button.colour.w = 1.0f;

			if (ThinDrawer::object->wh->mouseData[2] == 2)
			{
				CharDrawer::selectedLanguageIndex = i;
			}
		}

		button.colour.w *= size01;

		vkCmdPushConstants(cmdBuff, td->menuButton->pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_menuButton), &button);

		vkCmdDraw(cmdBuff, 6, 1, 0, 0);

		i++;
	}

	CharDrawer::draw(cmdBuff, Leveler::object->ol->cd);
}

glm::vec4 vendorIdToColor(uint32_t vendorId)
{
	switch (vendorId)
	{
	case 0x1002: // AMD
		return glm::vec4(0.5f, 0.05f, 0.0f, 0.5f);
	case 0x10DE: // NVIDIA
		return glm::vec4(0.0f, 0.5f, 0.0f, 0.5f);
	case 0x8086: // INTEL
		return glm::vec4(0.0f, 0.4f, 0.9f, 0.5f);
	case 0x106B: // APPLE
		return glm::vec4(0.0f, 0.0f, 0.0f, 0.5f);
	default:
		return glm::vec4(0.0f, 1.0f, 1.0f, 0.5f);
	}
}

static std::vector<int> actualIndices;
static s_lineRenderData bottomText;

void PopUp::initGpu()
{
	finalSize = std::min(ThinDrawer::object->deviceProperties.size() * 7.0f + 7.0f, 70.0f);

	finalButton = {
		glm::vec4(0.0f, 0.0f, 100.0f, finalSize), // posSize
		glm::vec4(0.04, 0.04, 0.08, 1.0), // clr
		glm::vec4(5.0f, 5.5f, 1.1f, 1.0f) }; // data

	renderTextDatas.clear();

	for (int i = 0; i < ThinDrawer::object->deviceProperties.size(); i++)
	{
		auto& prop = ThinDrawer::object->deviceProperties[i].prop;
		bool ignore = ThinDrawer::object->deviceProperties[i].ignore;

		if (ignore)
		{
			continue;
		}

		s_lineRenderData lrd;
		lrd.charDatas = Leveler::object->ol->cd->getRenderData(prop.deviceName, Leveler::object->ol->popHeight * 2, lrd.posScale.w);
		lrd.color = glm::vec4(1.0f);
		lrd.posScale = glm::vec4(-lrd.posScale.w * 2.5f, 0.0f, 5.0f, lrd.posScale.w);
		renderTextDatas.push_back(lrd);

		actualIndices.push_back(i);
	}

	std::string preText = Game::object->menu->buttonTexts["<currentGpu_" + std::to_string(Leveler::object->ol->cd->selectedLanguageIndex) + ">"][0];
	std::string fullText = preText + ": " + ThinDrawer::object->selectedPhysicalDeviceName;
	bottomText.charDatas = Leveler::object->ol->cd->getRenderData(fullText, Leveler::object->ol->popHeight * 2, bottomText.posScale.w);
	bottomText.color = glm::vec4(1.0f);
	bottomText.posScale = glm::vec4(-bottomText.posScale.w * 2.5f, 0.0f, 5.0f, bottomText.posScale.w);
}

void PopUp::gpuRender(VkCommandBuffer cmdBuff, void* ptr)
{
	ThinDrawer* td = ThinDrawer::object;
	PopUp* p = (PopUp*)ptr;

	const uint32_t camOffset = ShaderBase::alignedSharedSize * (td->frameIndex + 8);

	double leftTime = glm::mod(ThinDrawer::object->gameTime, 3.0);
	int index = std::clamp((int)leftTime, 0, 2);
	int nextIndex = (index + 1) % 3;
	int nextNextIndex = (index + 2) % 3;

	float nextClr = glm::fract(leftTime);
	float thisClr = 1.0 - nextClr;

	p->finalButton.colour[index] = thisClr * 0.02;
	p->finalButton.colour[nextIndex] = nextClr * 0.02;
	p->finalButton.colour[nextNextIndex] = 0.0f;

	vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, td->menuButton->pipeline);

	vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
		td->menuButton->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

	float td01 = std::min((td->gameTime - p->startTime) / p->animationTime, 1.0);
	td01 = (sin(td01 * glm::pi<float>() - glm::pi<float>() * 0.5f) + 1.0f) * 0.5f;

	s_menuButton currentButtonData;
	currentButtonData.colour = p->initialButton.colour + (p->finalButton.colour - p->initialButton.colour) * td01;
	currentButtonData.data = p->initialButton.data + (p->finalButton.data - p->initialButton.data) * td01;
	currentButtonData.posSize = p->initialButton.posSize + (p->finalButton.posSize - p->initialButton.posSize) * td01;

	float size01 = currentButtonData.posSize.w / finalSize;

	if (Game::object->gameStater == &Game::MenuState)
	{
		s_menuButton bgView;
		bgView.colour = glm::vec4(0.0f, 0.0f, 0.0f, size01);
		bgView.posSize = glm::vec4(0.0f, 0.0f, Leveler::object->ol->cam.xSides.y * 2.0f, Leveler::object->ol->cam.ySides.y * 2.0f);
		bgView.data = glm::vec4(0.0f, 0.0f, 1.1f, 1.0f);

		vkCmdPushConstants(cmdBuff, td->menuButton->pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_menuButton), &bgView);

		vkCmdDraw(cmdBuff, 6, 1, 0, 0);
	}

	vkCmdPushConstants(cmdBuff, td->menuButton->pipelineLayout,
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_menuButton), &currentButtonData);

	vkCmdDraw(cmdBuff, 6, 1, 0, 0);

	if (td01 > 0.999f && p->closeOnEnd)
	{
		Game::object->singleTaskRegistry.push_back([p]
		{
			ThinDrawer::object->eraseFromRenderFunctions(ThinDrawer::object->lateDrawFuncs, p);
			p->active = false;
		});
	}

	if (td->wh->keyData[SDL_SCANCODE_ESCAPE] == 2 ||
		!currentButtonData.hoverCheck(Leveler::object->ol->cam.getMouseCoords()) && td->wh->mouseData[2] == 2)
	{
		setClose(p, &currentButtonData);
	}

	std::string topText = Game::object->menu->buttonTexts["<gpuset_" + std::to_string(Leveler::object->ol->cd->selectedLanguageIndex) + ">"][0];

	float topStart = currentButtonData.posSize.y + currentButtonData.posSize.w * 0.5f;

	s_lineRenderData lrd;
	lrd.charDatas = Leveler::object->ol->cd->getRenderData(topText, Leveler::object->ol->popHeight, lrd.posScale.w);
	lrd.color = glm::vec4(0.3f, 0.0f, 0.0f, size01);
	lrd.posScale = glm::vec4(-lrd.posScale.w * 1.25f * size01, topStart, 2.5f * size01, 0.0f);

	Leveler::object->ol->cd->renderDatas.push_back(lrd);

	topStart -= 6.0f;

	int i = 0;
	for (auto& lrd : renderTextDatas)
	{
		float sizeRatio = std::min(19.0f / lrd.posScale.w, 1.0f);

		lrd.posScale.x = -lrd.posScale.w * 2.5f * size01 * sizeRatio;
		lrd.posScale.y = topStart;
		lrd.posScale.z = size01 * 5.0f * sizeRatio;
		lrd.color.w = size01;
		topStart -= 7.0f * size01;

		int index = actualIndices[i];
		auto& prop = ThinDrawer::object->deviceProperties[i].prop;

		Leveler::object->ol->cd->renderDatas.push_back(lrd);

		s_menuButton button;
		if (index == ThinDrawer::object->selectedPhysicalDeviceIndex && ThinDrawer::object->selectedVendorAndDeviceId != 0)
		{
			button.colour = glm::vec4(1.0f, 1.0f, 1.0f, 0.5f);
		}
		else
		{
			button.colour = vendorIdToColor(prop.vendorID);
		}

		button.data = glm::vec4(2.5f, 3.0f, 1.1f, 0.0f) * size01;
		button.data.w = 1.0;
		button.posSize = glm::vec4(0.0f, lrd.posScale.y, 97.5f * size01, 3.5f * size01);

		if (button.hoverCheck(Leveler::object->ol->cam.getMouseCoords()) && p->active)
		{
			button.colour.w = 1.0f;

			if (ThinDrawer::object->wh->mouseData[2] == 2)
			{
				if (ThinDrawer::object->selectedPhysicalDeviceIndex != index || ThinDrawer::object->selectedVendorAndDeviceId == 0)
				{
					ThinDrawer::object->selectedPhysicalDeviceIndex = index;
					ThinDrawer::object->selectedVendorAndDeviceId = (uint64_t)prop.deviceID | ((uint64_t)prop.vendorID << 32);
				}
				else
				{
					ThinDrawer::object->selectedVendorAndDeviceId = 0;
				}
			}
		}

		button.colour.w *= size01;

		vkCmdPushConstants(cmdBuff, td->menuButton->pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_menuButton), &button);

		vkCmdDraw(cmdBuff, 6, 1, 0, 0);

		i++;
	}

	float sizeRatio = std::min(24.0f / bottomText.posScale.w, 1.0f);
	bottomText.posScale.x = -bottomText.posScale.w * 2.0f * size01 * sizeRatio;
	bottomText.posScale.y = topStart;
	bottomText.posScale.z = size01 * 4.0f * sizeRatio;
	bottomText.color.w = size01;
	Leveler::object->ol->cd->renderDatas.push_back(bottomText);

	CharDrawer::draw(cmdBuff, Leveler::object->ol->cd);
}

void PopUp::initDarkness()
{
	float w;
	float r;
	if (initialButton.colour.w < 0.01f)
	{
		w = 1.0f;
		r = 1.5f;
	}
	else
	{
		w = 0.0f;
		r = 0.0f;
	}

	finalButton = {
		glm::vec4(0.0f, 0.0f, 300.0f, 300.0f), // posSize
		glm::vec4(0.0f, 0.0f, 0.0f, w), // clr
		glm::vec4(5.0f, 5.5f, r, 0.1f) }; // data

	renderTextDatas.clear();
}

void PopUp::darknessRender(VkCommandBuffer cmdBuff, void* ptr)
{
	ThinDrawer* td = ThinDrawer::object;
	PopUp* p = (PopUp*)ptr;

	const uint32_t camOffset = ShaderBase::alignedSharedSize * (td->frameIndex + 8);

	vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, td->menuButton->pipeline);

	vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
		td->menuButton->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

	float td01 = std::min((td->gameTime - p->startTime) / p->animationTime, 1.0);
	td01 = (sin(td01 * glm::pi<float>() - glm::pi<float>() * 0.5f) + 1.0f) * 0.5f;

	s_menuButton currentButtonData;
	currentButtonData.colour = p->initialButton.colour + (p->finalButton.colour - p->initialButton.colour) * td01;
	currentButtonData.data = p->initialButton.data + (p->finalButton.data - p->initialButton.data) * td01;
	currentButtonData.posSize = p->initialButton.posSize + (p->finalButton.posSize - p->initialButton.posSize) * td01;

	vkCmdPushConstants(cmdBuff, td->menuButton->pipelineLayout,
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_menuButton), &currentButtonData);

	vkCmdDraw(cmdBuff, 6, 1, 0, 0);

	if (td01 > 0.999f)
	{
		if (currentButtonData.colour.w < 0.01f)
		{
			Game::object->singleTaskRegistry.push_back([p]
			{
				p->active = false;
				ThinDrawer::object->eraseFromRenderFunctions(ThinDrawer::object->lateDrawFuncs, p);

				p->endFunction.reset();
			});
		}

		if (p->endFunction.has_value())
		{
			p->active = false;

			p->endFunction.value()();
			p->endFunction.reset();
		}
	}
}

static float maxScroll = 0.0f;
static bool canCloseOnEnd = true;

void PopUp::initCredits()
{
	finalButton = {
		glm::vec4(0.0f, 0.0f, 100.0f, 100.0f), // posSize
		glm::vec4(0.0f, 0.0f, 0.0f, 0.9f), // clr
		glm::vec4(200.0f, 250.0f, 1.5f, 0.2f) }; // data

	scrollAmount = -50.0f;
	targetScroll = -50.0f;

	canCloseOnEnd = true;

	maxScroll = 0.0f;
	Menu* m = Game::object->menu;
	for (int i = 0; ; i++)
	{
		std::string stri = std::to_string(i);
		std::string searchText = "<" + stri + "_" + std::to_string(CharDrawer::selectedLanguageIndex) + ">";

		auto iter = m->creditsTexts.find(searchText);

		if (iter == m->creditsTexts.end())
		{
			break;
		}

		auto vec = iter->second;

		if (vec.size() < 2) continue;

		maxScroll += 8.0f;
		maxScroll += (vec.size() - 1) * 6.0f;

		maxScroll += 5.0f;
	}

	renderTextDatas.clear();
}

void PopUp::creditsRender(VkCommandBuffer cmdBuff, void* ptr)
{
	ThinDrawer* td = ThinDrawer::object;
	PopUp* p = (PopUp*)ptr;
	Menu* m = Game::object->menu;

	const uint32_t camOffset = ShaderBase::alignedSharedSize * (td->frameIndex + 8);

	vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, td->menuButton->pipeline);

	vkCmdBindDescriptorSets(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
		td->menuButton->pipelineLayout, 0, 1, &ShaderBase::sharedSet, 1, &camOffset);

	float td01 = std::min((td->gameTime - p->startTime) / p->animationTime, 1.0);
	td01 = (sin(td01 * glm::pi<float>() - glm::pi<float>() * 0.5f) + 1.0f) * 0.5f;

	s_menuButton currentButtonData;
	currentButtonData.colour = p->initialButton.colour + (p->finalButton.colour - p->initialButton.colour) * td01;
	currentButtonData.data = p->initialButton.data + (p->finalButton.data - p->initialButton.data) * td01;
	currentButtonData.posSize = p->initialButton.posSize + (p->finalButton.posSize - p->initialButton.posSize) * td01;

	vkCmdPushConstants(cmdBuff, td->menuButton->pipelineLayout,
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s_menuButton), &currentButtonData);

	vkCmdDraw(cmdBuff, 6, 1, 0, 0);

	if (p->closeOnEnd && td01 > 0.999f)
	{
		Game::object->singleTaskRegistry.push_back([p]
			{
				p->active = false;
				ThinDrawer::object->eraseFromRenderFunctions(ThinDrawer::object->lateDrawFuncs, p);
			});
	}

	if (td->wh->keyData[SDL_SCANCODE_ESCAPE] == 2 || (canCloseOnEnd && p->scrollAmount + 0.01f > maxScroll))
	{
		setClose(p, &currentButtonData);
	}
	
	const float camUp = Leveler::object->ol->cam.ySides.y;
	const float sizeMult = std::max(currentButtonData.posSize.z - 50.0f, 0.0f) / 50.0f;

	const float yLimitUp = camUp - 10.0f;
	const float yLimitDown = -camUp;

	float currentY = camUp - 5.0f;

	std::string creditsText = m->creditsTexts["<title>"][CharDrawer::selectedLanguageIndex];

	s_lineRenderData lrd;
	lrd.charDatas = Leveler::object->ol->cd->getRenderData(creditsText, Leveler::object->ol->popHeight * 2, lrd.posScale.w);
	lrd.color = glm::vec4(sizeMult);
	lrd.posScale = glm::vec4(-lrd.posScale.w * 2.5f * sizeMult, currentY, 5.0f * sizeMult, lrd.posScale.w);
	currentY -= 10.0f;

	Leveler::object->ol->cd->renderDatas.push_back(lrd);

	std::string selectedLangString = std::to_string(CharDrawer::selectedLanguageIndex);

	if (td->wh->mouseData[5] != 0.0f)
	{
		canCloseOnEnd = false;
		p->targetScroll -= td->wh->mouseData[5] * 10.0;
	}

	p->targetScroll += td->deltaTime * 10.0f;

	p->scrollAmount += (p->targetScroll - p->scrollAmount) * td->deltaTime * 10.0f;

	p->targetScroll = std::clamp(p->targetScroll, -50.0f, maxScroll);
	p->scrollAmount = std::clamp(p->scrollAmount, -50.0f, maxScroll);

	currentY += p->scrollAmount;

	for (int i = 0; ; i++)
	{
		std::string stri = std::to_string(i);
		std::string searchText = "<" + stri + "_" + selectedLangString + ">";

		auto iter = m->creditsTexts.find(searchText);

		if (iter == m->creditsTexts.end())
		{
			break;
		}

		auto vec = iter->second;

		if (vec.size() < 2) continue;

		std::string clrsSearch = "<colour_" + stri + ">";
		glm::vec4 clr;
		
		iter = m->creditsTexts.find(clrsSearch);

		if (iter != m->creditsTexts.end() && iter->second.size() == 4)
		{
			auto clrVec = iter->second;
			for (int j = 0; j < 4; j++)
			{
				clr[j] = std::stof(clrVec[j]) / 2048.0f;
			}
		}
		else
		{
			clr = glm::vec4(1.0f);
		}

		for (int j = 0; j < vec.size(); j++)
		{
			float m2 = j == 0 ? 2.0f : 1.5f;

			if (currentY > yLimitUp || currentY < yLimitDown)
			{
				currentY -= 4.0f * m2;
				continue;
			}

			float upDownMult = sqrt(std::min(std::min(std::abs(yLimitUp - currentY), std::abs(yLimitDown - currentY)) * 0.1f, 1.0f));

			lrd.color = clr * upDownMult;
			upDownMult = std::min(upDownMult + 0.3f, 1.0f);
			lrd.charDatas = Leveler::object->ol->cd->getRenderData(vec[j], Leveler::object->ol->popHeight * m2, lrd.posScale.w);
			lrd.posScale = glm::vec4(-lrd.posScale.w * 1.25f * sizeMult * m2 * upDownMult, currentY, 2.5f * sizeMult * m2 * upDownMult, lrd.posScale.w);
			Leveler::object->ol->cd->renderDatas.push_back(lrd);
			currentY -= 4.0f * m2;
		}

		currentY -= 5.0f;
	}

	CharDrawer::draw(cmdBuff, Leveler::object->ol->cd);
}