#pragma once

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#include <AudioFile.h>
#include <map>
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "GameObjects/Receiver.h"

#define NUM_LOOPING_SOURCES 32
#define NUM_ONETIME_SOURCES 256 - NUM_LOOPING_SOURCES
#define SQUARE_SIZE 4

typedef struct s_sourceInUse
{
	s_sourceInUse() { }

	ALuint source;
	s_sourceInUse* next;
} s_sourceInUse;

typedef struct
{
	ALuint source;
	double startTime;
	double reachTime;
	float initialVolume;
	float finalVolume;
	bool stopOnEnd;
} fader;

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

	std::vector<fader> faderData;

	std::vector<std::vector<std::vector<gridSoundData*>>> statGrid;
	std::vector<gridSoundData*> statPlays;

	static Sounder* object;

	static float generalVolume;
	static float musicVolume;
	static float effectVolume;

	ALCdevice* device;
	ALCcontext* context;
	std::map<std::string, ALuint> bufferMap;

	glm::vec3 listenerPos = glm::vec3(0.0, 0.0, 0.0);
	float soundRadius = 10.0f;

	Sounder();
	~Sounder();

	void updateStationaries();
	void update();

	void gridClear();
	void putStatOnGrid(gridSoundData* soundData, std::string fileName, float volume, bool enabled, bool isPoint);
	void putStatOnGrid(gridSoundData* data);
	glm::ivec2 getSoundGridPos(glm::vec2 pos);

	ALuint loadSound(std::string fileName);
	ALuint playSound(ALuint buffer, float volume);
	void setSoundVolume(const ALuint source, const float volume);
	void setSoundPosition(const ALuint source, glm::vec3 pos);
	ALuint playLoopingSound(ALuint buffer, float volume);
	void stopSound(ALuint source);
	void stopLoopingSound(ALuint source);

	void addFader(ALuint source, float initialVolume, float finalVolume, double reachTime, bool stopOnEnd = false);

	static glm::vec2 getSoundPlayPosition(glm::vec2 linePoint1, glm::vec2 linePoint2, glm::vec2 point);
};