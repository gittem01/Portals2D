#include <Sounder.h>
#include <IOHelper.h>
#include <GLFW/glfw3.h>

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

    // load default
    loadSound("portalRadio.wav");
    loadSound("jumpTest.wav");
    loadSound("wav_stereo.wav");
    loadSound("blueShoot.wav");
    loadSound("orangeShoot.wav");

    //ALuint source = playLoopingSound(radio);

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
}

ALuint Sounder::loadSound(std::string fileName)
{
    auto iter = bufferMap.find(fileName);
    if (iter != bufferMap.end())
    {
        return iter->second;
    }

    AudioFile<short> audioFile;
    
    audioFile.load(IOHelper::assetsFolder + "/sounds/" + fileName);
    audioFile.printSummary();

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

ALuint Sounder::playSound(ALuint buffer)
{
    if (lastAvailableOnetimeSource >= NUM_ONETIME_SOURCES)
    {
        return 0;
    }

    ALuint source = onetimeSources[lastAvailableOnetimeSource];
    s_sourceInUse* newSource = availableSourcesInUse[lastAvailableOnetimeSource];
    lastAvailableOnetimeSource++;

    alSourcei(source, AL_BUFFER, buffer);
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

ALuint Sounder::playLoopingSound(ALuint buffer)
{
    ALuint source = loopingSources[lastAvailableLoopingSource++];
    alSourcei(source, AL_BUFFER, buffer);
    alSourcei(source, AL_LOOPING, AL_TRUE);
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