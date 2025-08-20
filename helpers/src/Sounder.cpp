#include <Sounder.h>
#include <IOHelper.h>
#include "ThinDrawer.h"
#include <Camera.h>

Sounder* Sounder::object;

float Sounder::generalVolume = 0.5f;
float Sounder::musicVolume = 0.5f;
float Sounder::effectVolume = 0.5f;

Sounder::Sounder()
{
    device = alcOpenDevice(nullptr);
    context = alcCreateContext(device, nullptr);
    alcMakeContextCurrent(context);

    ALfloat listenerPosition[] = { 0.0f, 0.0f, 0.0f };
    alListenerfv(AL_POSITION, listenerPosition);

    lastAvailableOnetimeSource = 0;
    lastAvailableLoopingSource = 0;

    alGenSources(NUM_ONETIME_SOURCES, onetimeSources);
    alGenSources(NUM_LOOPING_SOURCES, loopingSources);

    memcpy(onetimeSourcesCopy, onetimeSources, sizeof(onetimeSources));
    memcpy(loopingSourcesCopy, loopingSources, sizeof(loopingSources));

    for (int i = 0; i < NUM_LOOPING_SOURCES; i++)
    {
        alSourcei(loopingSources[i], AL_LOOPING, AL_TRUE);
    }

    for (int i = 0; i < NUM_ONETIME_SOURCES; i++)
    {
        availableSourcesInUse[i] = &sourcesInUseAllocator[i];
    }

    sourcesInUse = nullptr;
    sourceToCheck = nullptr;

    Sounder::object = this;

    alListenerf(AL_GAIN, 20.0);

    // load default
    loadSound("jumpTest.wav");
    loadSound("blueGelJump.wav");
    loadSound("blueShoot.wav");
    loadSound("orangeShoot.wav");
    loadSound("blueShootJ.wav");
    loadSound("orangeShootJ.wav");
    loadSound("portalFizzle.wav");
    loadSound("leveler.wav");
    loadSound("inGame.wav");
    loadSound("walkloop.wav");

    statGrid.resize(400 / SQUARE_SIZE);
    for (int i = 0; i < statGrid.size(); i++)
    {
        statGrid[i].resize(400 / SQUARE_SIZE);
    }

    // device switch
    bool hasReopenDeviceExtension = alcIsExtensionPresent(device, "ALC_SOFT_reopen_device");
    LPALCREOPENDEVICESOFT alcReopenDeviceSOFT = reinterpret_cast<LPALCREOPENDEVICESOFT>(alcGetProcAddress(device, "alcReopenDeviceSOFT"));
    alcReopenDeviceSOFT(device, nullptr, nullptr);
}

Sounder::~Sounder()
{
    alcMakeContextCurrent(nullptr);
    alcDestroyContext(context);
    alcCloseDevice(device);
}

inline glm::ivec2 Sounder::getSoundGridPos(glm::vec2 pos)
{
    glm::ivec2 newPos = glm::ivec2((pos + 200.0f) / (float)SQUARE_SIZE);
    newPos.y = 400 / SQUARE_SIZE - 1 - newPos.y;
    newPos = glm::clamp(newPos, 0, 400 / SQUARE_SIZE - 1);

    return newPos;
}

void Sounder::gridClear()
{
	for (int i = 0; i < statPlays.size(); i++)
	{
		stopSound(statPlays[i]->source);
        alSourcei(statPlays[i]->source, AL_LOOPING, AL_FALSE);
	}

    statPlays.clear();

    for (int i = 0; i < statGrid.size(); i++)
    {
        for (int j = 0; j < statGrid[i].size(); j++)
        {
            statGrid[i][j].clear();
        }
    }
}

void Sounder::putStatOnGrid(gridSoundData* soundData, std::string fileName, float volume, bool enabled, bool isPoint)
{
    soundData->enabled = enabled;
    soundData->isPoint = isPoint;
    soundData->playing = false;
    soundData->source = 0;
    soundData->buffer = Sounder::object->loadSound(fileName);
    soundData->baseVolume = volume;
    soundData->currentVolume = 0.0f;

    putStatOnGrid(soundData);
}

void Sounder::putStatOnGrid(gridSoundData* data)
{
    glm::ivec2 pos0 = getSoundGridPos(data->pos0);

    if (data->isPoint)
    {
		statGrid[pos0.y][pos0.x].push_back(data);
	}
    else
    {
        glm::ivec2 pos1 = getSoundGridPos(data->pos1);

        if (pos0.x != pos1.x && pos1.y != pos1.y)
        {
            if (abs(pos0.x - pos1.x) > abs(pos0.y - pos1.y))
            {
                pos0.y = pos1.y;
            }
            else
            {
                pos0.x = pos1.x;
            }
        }

		int incX = pos0.x < pos1.x ? 1 : -1;
		int incY = pos0.y < pos1.y ? 1 : -1;

		for (int x = pos0.x; x != pos1.x + incX; x += incX)
		{
			for (int y = pos0.y; y != pos1.y + incY; y += incY)
			{
				statGrid[y][x].push_back(data);
			}
		}
    }
}

static const float checkSizeMult = 2.0f;
static const float checkSizeAdd = 12.0f;

void Sounder::updateStationaries()
{
    uint64_t timeStart = SDL_GetTicksNS();

    glm::ivec2 gridSquare = getSoundGridPos(listenerPos);

	float camXSize = ThinDrawer::object->wh->cam->xSides.y - ThinDrawer::object->wh->cam->xSides.x;
	float camYSize = ThinDrawer::object->wh->cam->ySides.y - ThinDrawer::object->wh->cam->ySides.x;

	soundRadius = 0.5f * glm::max(camXSize, camYSize) * checkSizeMult + checkSizeAdd;
    auto add_vec = glm::ivec2((int)(soundRadius / SQUARE_SIZE));

	glm::ivec2 gridStart = gridSquare - add_vec;
	glm::ivec2 gridEnd = gridSquare + add_vec;

    glm::ivec2 scanStart = glm::clamp(gridStart, 0, 400 / SQUARE_SIZE - 1);
	glm::ivec2 scanEnd = glm::clamp(gridEnd, 0, 400 / SQUARE_SIZE - 1);

    glm::vec2 camPos = glm::vec2(listenerPos.x, -listenerPos.y);

	for (int x = scanStart.x; x <= scanEnd.x; x++)
	{
		for (int y = scanStart.y; y <= scanEnd.y; y++)
		{
			auto& vec = statGrid[y][x];

            for (int i = 0; i < vec.size(); i++)
            {
                gridSoundData* soundData = vec[i];

                if (!soundData->enabled || soundData->playing)
                {
					continue;
                }

                glm::vec2 closestPos;
                if (!soundData->isPoint)
                {
                    closestPos = getSoundPlayPosition(soundData->pos0, soundData->pos1, camPos);
                }
                else
                {
                    closestPos = soundData->pos0;
                }

				float len = glm::length(closestPos - camPos);

                if (len > soundRadius)
                {
                    continue;
                }

                ALuint source = playSound(soundData->buffer, soundData->currentVolume + 0.001);

                if (source > 0)
                {
                    soundData->source = source;
                    soundData->playing = true;
                    alSourcei(soundData->source, AL_LOOPING, AL_TRUE);

                    statPlays.push_back(soundData);
                }
            }
		}
	}

    for (int i = 0; i < statPlays.size(); i++)
    {
        gridSoundData* soundData = statPlays[i];

        glm::vec2 closestPos;
        if (!soundData->isPoint)
        {
            closestPos = getSoundPlayPosition(soundData->pos0, soundData->pos1, camPos);
        }
        else
        {
			closestPos = soundData->pos0;
        }

        float len = glm::length(closestPos - camPos);

        float soundAdd = soundData->baseVolume * ThinDrawer::object->deltaTime;

        if (len > soundRadius || !soundData->enabled)
        {
            if (!soundData->enabled)
            {
				soundData->currentVolume = glm::max(0.0f, soundData->currentVolume - soundAdd);

                if (soundData->currentVolume > 0.001f)
                {
                    goto ifOut;
                }
            }

            alSourcei(soundData->source, AL_LOOPING, AL_FALSE);
            stopSound(soundData->source);

            soundData->currentVolume = 0.0f;
            statPlays[i] = statPlays.back();
            statPlays.pop_back();

            soundData->playing = false;

            i--;

            continue;
        }

ifOut:

        if (soundData->enabled)
        {
			soundData->currentVolume += soundAdd;
			soundData->currentVolume = glm::min(soundData->currentVolume, soundData->baseVolume);
        }

        float soundMult = glm::clamp(1.0f - len / soundRadius, 0.0f, 1.0f);

        const float setVolume = setSoundVolume(soundData->source, soundData->currentVolume * soundMult);
        if (setVolume > 0.0f)
        {
            setSoundPosition(soundData->source, glm::vec3(closestPos.x, closestPos.y, 0.0));
            ThinDrawer::object->registerDebugLine(glm::vec4(closestPos.x, closestPos.y, closestPos.x, closestPos.y), glm::vec4(1.0f), 0.2f, 0.5f);
        }
        else
        {
            ThinDrawer::object->registerDebugLine(glm::vec4(closestPos.x, closestPos.y, closestPos.x, closestPos.y), glm::vec4(1.0f, 0.0, 0.0, 1.0), 0.2f, 0.5f);
        }
    }

    uint64_t timeEnd = SDL_GetTicksNS();

    //printf("Time ms: %f\n", (timeEnd - timeStart) / 1000000.0);
}

void Sounder::update()
{
    //const ALCchar* defaultDeviceSpecifier = alcGetString(device, ALC_ALL_DEVICES_SPECIFIER);
    //printf("%s\n", defaultDeviceSpecifier);

    static float lastMusicVolume = 0.0;

    float currentMusicVolume = musicVolume * generalVolume;

    if (lastMusicVolume != musicVolume)
    {
        for (int i = 0; i < lastAvailableLoopingSource; i++)
        {
            alSourcef(loopingSources[i], AL_GAIN, 0.1f * currentMusicVolume);
        }
    }

    lastMusicVolume = currentMusicVolume;

    alListenerfv(AL_POSITION, &listenerPos.x);

    if (sourceToCheck)
    {
        s_sourceInUse* src;
        s_sourceInUse* prevSrc;
        if (sourceToCheck->next == nullptr)
        {
            src = sourcesInUse;
            prevSrc = nullptr;
        }
        else
        {
            src = sourceToCheck->next;
            prevSrc = sourceToCheck;
        }

        ALint sourceState;
        alGetSourcei(src->source, AL_SOURCE_STATE, &sourceState);

        if (sourceState == AL_STOPPED)
        {
            alSourceStop(src->source);
            alSourcef(src->source, AL_GAIN, 1.0);
            alSourcei(src->source, AL_SOURCE_RELATIVE, AL_FALSE);
            alSource3f(src->source, AL_POSITION, 0.0f, 0.0f, 0.0f);

            if (!prevSrc)
            {
                sourcesInUse = src->next;
            }
            else
            {
                prevSrc->next = src->next;
            }

            if (src->next == nullptr)
            {
                sourceToCheck = sourcesInUse;
            }
            else
            {
                sourceToCheck = src->next;
            }

            --lastAvailableOnetimeSource;
            onetimeSources[lastAvailableOnetimeSource] = src->source;
            availableSourcesInUse[lastAvailableOnetimeSource] = src;
        }
        else
        {
            sourceToCheck = src;
        }
    }

    for (int i = 0; i < faderData.size(); i++)
    {
		fader f = faderData[i];
		double dt01 = (ThinDrawer::object->gameTime - f.startTime) / f.reachTime;
        float finalVolume = (f.initialVolume + (f.finalVolume - f.initialVolume) * dt01) * generalVolume * musicVolume;

        if (dt01 >= 1.0)
        {
            if (f.stopOnEnd)
            {
                stopLoopingSound(f.source);
            }

            faderData[i] = faderData.back();
            faderData.pop_back();

            i--; continue;
        }
        else
        {
            alSourcef(f.source, AL_GAIN, finalVolume);
        }
    }

    updateStationaries();
}

void Sounder::addFader(ALuint source, float initialVolume, float finalVolume, double reachTime, bool stopOnEnd)
{
    for (int i = 0; i < faderData.size(); i++)
    {
		if (faderData[i].source == source)
		{
            faderData[i] = { source, ThinDrawer::object->gameTime, reachTime, initialVolume, finalVolume, stopOnEnd };
            return;
		}
    }

	faderData.push_back({ source, ThinDrawer::object->gameTime, reachTime, initialVolume, finalVolume, stopOnEnd });
}

ALuint Sounder::loadSound(std::string fileName)
{
    auto iter = bufferMap.find(fileName);
    if (iter != bufferMap.end())
    {
        return iter->second;
    }

    AudioFile<short> audioFile;
    
    std::string path = IOHelper::assetsFolder + "sounds/" + fileName;
    
    audioFile.load(path);

    if (audioFile.getNumSamplesPerChannel() == 0) return 0;

    ALvoid* data;
    ALsizei size;
    if (audioFile.getBitDepth() == 8)
    {
        for (int i = 0; i < audioFile.samples[0].size(); i++)
        {
            audioFile.samples[0][i] = audioFile.samples[0][i] * 256;
        }
    }

    ALuint buffer;
    alGenBuffers(1, &buffer);
    alBufferData(buffer, AL_FORMAT_MONO16, audioFile.samples[0].data(), audioFile.samples[0].size() * sizeof(ALshort), audioFile.getSampleRate());
    
    bufferMap[fileName] = buffer;

    return buffer;
}

ALuint Sounder::playSound(ALuint buffer, float volume)
{
    if (lastAvailableOnetimeSource >= NUM_ONETIME_SOURCES)
    {
        return 0;
    }

	float setVolume = volume * generalVolume * effectVolume;
	if (setVolume <= 0.0f) return 0;

    ALuint source = onetimeSources[lastAvailableOnetimeSource];
    s_sourceInUse* newSource = availableSourcesInUse[lastAvailableOnetimeSource];
    lastAvailableOnetimeSource++;

    alSourcei(source, AL_BUFFER, buffer);
    alSourcef(source, AL_GAIN, setVolume);
    alSourcePlay(source);

    newSource->source = source;
    newSource->next = sourcesInUse;
    if (!sourcesInUse)
    {
        sourceToCheck = newSource;
    }
    sourcesInUse = newSource;

    return source;
}

float Sounder::setSoundVolume(ALuint source, const float volume)
{
    const float volumeSet = volume * generalVolume * effectVolume;
    alSourcef(source, AL_GAIN, volumeSet);

    return volumeSet;
}

void Sounder::setSoundPosition(ALuint source, glm::vec3 pos)
{
    alSource3f(source, AL_POSITION, pos.x, pos.y, pos.z);
}

ALuint Sounder::playLoopingSound(ALuint buffer, float volume)
{
    ALuint source = loopingSources[lastAvailableLoopingSource++];
    alSourcei(source, AL_BUFFER, buffer);
    alSourcei(source, AL_LOOPING, AL_TRUE);
    alSourcei(source, AL_SOURCE_RELATIVE, AL_TRUE);
    alSourcef(source, AL_GAIN, volume * musicVolume * generalVolume);
    alSourcePlay(source);

    return source;
}

void Sounder::stopSound(ALuint source)
{
    alSourceStop(source);
}

void Sounder::stopLoopingSound(ALuint source)
{
    alSourceStop(source);

    for (int i = 0; i < lastAvailableLoopingSource; i++)
    {
        if (loopingSources[i] == source)
        {
            loopingSources[i] = loopingSources[--lastAvailableLoopingSource];
            loopingSources[lastAvailableLoopingSource] = source;
            return;
        }
    }
}

glm::vec2 Sounder::getSoundPlayPosition(glm::vec2 linePoint1, glm::vec2 linePoint2, glm::vec2 point)
{
    glm::vec2 pl1 = linePoint1 - point;
	glm::vec2 pl2 = linePoint2 - point;
    glm::vec2 p12 = linePoint2 - linePoint1;

	float length1 = glm::length(pl1);
	float length2 = glm::length(pl2);

	float ratio = length1 / (length1 + length2);

	glm::vec2 result = linePoint1 + p12 * ratio;

	return result;
}