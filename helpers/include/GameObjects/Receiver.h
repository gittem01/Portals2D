#pragma once

typedef struct
{
	uint32_t buffer;
	uint32_t source;
	glm::vec2 pos0;
	glm::vec2 pos1;
	glm::vec2 lastPlayPos;
	float baseVolume;
	float currentVolume;
	bool enabled;
	bool playing;
	bool isPoint;
} gridSoundData;

class Receiver
{	
public:
	gridSoundData soundData;

	char signalCount;
	char reqSignalCount;
	bool disabler;
	bool enabled;

	virtual void enable() { };
	virtual void disable() { };

	void ssignal()
	{
		signalCount++;

		if (signalCount == reqSignalCount)
		{
			if (disabler)
			{
				enabled = false;
				soundData.enabled = false;
				disable();
			}
			else
			{
				enabled = true;
				soundData.enabled = true;
				enable();
			}
		}
	}

	void usignal()
	{
		signalCount--;

		if (signalCount == reqSignalCount - 1)
		{
			if (disabler)
			{
				enabled = true;
				soundData.enabled = true;
				enable();
			}
			else
			{
				enabled = false;
				soundData.enabled = false;
				disable();
			}
		}
	}

	virtual void ssignal2() { };
	virtual void usignal2() { };
};