#pragma once

#include "LevelerTestCase.h"
#include <box2d/box2d.h>
#include <AL/al.h>

class Transmitter
{
public:
	static ALuint enablerSound;
	static ALuint disablerSound;

	std::vector<connectionSignaller> connections;
	bool enabled = false;

	static void enableSound(b2Vec2 pos)
	{
		ALuint sound = Sounder::object->playSound(enablerSound, 1.0f);
		Sounder::object->setSoundPosition(sound, glm::vec3(pos.x, pos.y, 0.0));
	}

	static void disableSound(b2Vec2 pos)
	{
		ALuint sound = Sounder::object->playSound(disablerSound, 1.0f);
		Sounder::object->setSoundPosition(sound, glm::vec3(pos.x, pos.y, 0.0));
	}

	void enable()
	{
		if (enabled) return;

		enabled = true;

		ThinDrawer* thinDrawer = Leveler::object->thinDrawer;

		for (auto& conn : connections)
		{
			if (conn.secondaryFunc)
				conn.receiver->ssignal2();
			else
				conn.receiver->ssignal();

			for (int offset : conn.offsets)
			{
				s_lineZUbo* lineZUbo;
				vkMapMemory(thinDrawer->logicalDevice, thinDrawer->lineZ->uniformBuffers[0].memory, offset * sizeof(s_lineZUbo),
					sizeof(s_lineZUbo), 0, (void**)&lineZUbo);
				lineZUbo->colour = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f);
				vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->lineZ->uniformBuffers[0].memory);
			}
		}
	}

	void disable()
	{
		if (!enabled) return;

		enabled = false;

		ThinDrawer* thinDrawer = Leveler::object->thinDrawer;

		for (auto& conn : connections)
		{
			if (conn.secondaryFunc)
				conn.receiver->usignal2();
			else
				conn.receiver->usignal();

			for (int offset : conn.offsets)
			{
				s_lineZUbo* lineZUbo;
				vkMapMemory(thinDrawer->logicalDevice, thinDrawer->lineZ->uniformBuffers[0].memory, offset * sizeof(s_lineZUbo),
					sizeof(s_lineZUbo), 0, (void**)&lineZUbo);
				lineZUbo->colour = glm::vec4(0.0f, 0.5f, 1.0f, 1.0f);
				vkUnmapMemory(thinDrawer->logicalDevice, thinDrawer->lineZ->uniformBuffers[0].memory);
			}
		}
	}
};