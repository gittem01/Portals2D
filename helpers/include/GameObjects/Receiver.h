#pragma once

class Receiver
{	
public:
	int signalCount = 0;

	virtual void ssignal() = 0;
	virtual void usignal() = 0;

	virtual void ssignal2() { };
	virtual void usignal2() { };
};