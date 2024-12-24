#include <Sounder.h>
#include <IOHelper.h>
#include "ThinDrawer.h"

Sounder* Sounder::object;

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
    ALuint src = loadSound("portalRadio.wav");
    loadSound("jumpTest.wav");
    loadSound("blueGelJump.wav");
    src = loadSound("wav_stereo.wav");
    loadSound("blueShoot.wav");
    loadSound("orangeShoot.wav");
    loadSound("blueShootJ.wav");
    loadSound("orangeShootJ.wav");
    loadSound("portalFizzle.wav");

    //playLoopingSound(src);

    // device switch
    /*bool hasReopenDeviceExtension = alcIsExtensionPresent(device, "ALC_SOFT_reopen_device");
    LPALCREOPENDEVICESOFT alcReopenDeviceSOFT = reinterpret_cast<LPALCREOPENDEVICESOFT>(alcGetProcAddress(device, "alcReopenDeviceSOFT"));
    alcReopenDeviceSOFT(device, nullptr, nullptr);*/
}

Sounder::~Sounder()
{
    alcMakeContextCurrent(nullptr);
    alcDestroyContext(context);
    alcCloseDevice(device);
}

void Sounder::update()
{
    //const ALCchar* defaultDeviceSpecifier = alcGetString(device2, ALC_ALL_DEVICES_SPECIFIER);
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
		fader& f = faderData[i];
		double dt01 = (ThinDrawer::object->gameTime - f.startTime) / f.reachTime;
        float finalVolume = (f.initialVolume + (f.finalVolume - f.initialVolume) * dt01) * generalVolume * musicVolume;

        if (dt01 >= 1.0)
        {
            faderData[i] = faderData.back();
            faderData.pop_back();

            if (f.stopOnEnd)
            {
                stopLoopingSound(f.source);
            }

            i--; continue;
        }
        else
        {
            alSourcef(f.source, AL_GAIN, finalVolume);
        }
    }
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

void Sounder::setSoundVolume(ALuint source, float volume)
{
    alSourcef(source, AL_GAIN, volume * generalVolume * effectVolume);
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
    loopingSources[--lastAvailableLoopingSource] = source;
}