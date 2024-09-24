#pragma once

class Receiver
{	
public:
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
				disable();
			}
			else
			{
				enabled = true;
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
				enable();
			}
			else
			{
				enabled = false;
				disable();
			}
		}
	}

	virtual void ssignal2() { };
	virtual void usignal2() { };
};