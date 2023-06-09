#pragma once

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#include <AudioFile.h>
#include <map>
#include <string>

#define NUM_LOOPING_SOURCES 32
#define NUM_ONETIME_SOURCES 256 - NUM_LOOPING_SOURCES

typedef struct s_sourceInUse
{
	s_sourceInUse() { }

	ALuint source;
	s_sourceInUse* next;
} s_sourceInUse;

class Sounder
{
public:	
	int lastAvailableOnetimeSource;
	int lastAvailableLoopingSource;

	s_sourceInUse sourcesInUseAllocator[NUM_ONETIME_SOURCES];
	s_sourceInUse* availableSourcesInUse[NUM_ONETIME_SOURCES];

	ALuint onetimeSources[NUM_ONETIME_SOURCES];
	ALuint loopingSources[NUM_LOOPING_SOURCES];

	ALuint onetimeSourcesCopy[NUM_ONETIME_SOURCES];
	ALuint loopingSourcesCopy[NUM_LOOPING_SOURCES];

	s_sourceInUse* sourcesInUse;
	s_sourceInUse* sourceToCheck;

public:
	static Sounder* object;

	ALCdevice* device;
	ALCcontext* context;
	std::map<std::string, ALuint> bufferMap;

	Sounder();
	~Sounder();

	void update();

	ALuint loadSound(std::string fileName);
	ALuint playSound(ALuint buffer, float volume);
	void setSoundVolume(const ALuint source, const float volume);
	ALuint playLoopingSound(ALuint buffer);
	void stopSound(ALuint source);
	void stopLoopingSound(ALuint source);
};