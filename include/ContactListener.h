#pragma once

#include "TestPlayer.h"
#include <box2d/box2d.h>
#include <stdio.h>
#include "Portal.h"

class ContactListener: public b2ContactListener
{
public:
    
    virtual void BeginContact(b2Contact* contact);

    virtual void EndContact(b2Contact* contact);

    virtual void PreSolve(b2Contact* contact, const b2Manifold* oldManifold);

	virtual void PostSolve(b2Contact* contact, const b2ContactImpulse* impulse);

    void endBeginHandle(contactType type, b2Contact* contact);
};