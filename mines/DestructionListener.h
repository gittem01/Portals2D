#pragma once

#include <box2d/box2d.h>

// later use
class DestrucionListener : public b2DestructionListener {
public:
	virtual void SayGoodbye(b2Joint* joint) {
		
	}


	virtual void SayGoodbye(b2Fixture* fixture) {

	}
};