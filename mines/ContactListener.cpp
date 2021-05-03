#include "ContactListener.h"
#include "Portal.h"

void ContactListener::BeginContact(b2Contact* contact){
    for (Portal* portal : Portal::portals) {
        if (portal->midFixture == contact->GetFixtureA() || portal->collisionSensor == contact->GetFixtureA()) {
            portal->handleCollision(contact->GetFixtureB(), contact->GetFixtureA(), contact, BEGIN_CONTACT);
        }
        else if (portal->midFixture == contact->GetFixtureB() || portal->collisionSensor == contact->GetFixtureB()) {
            portal->handleCollision(contact->GetFixtureA(), contact->GetFixtureB(), contact, BEGIN_CONTACT);
        }
    }
}

void ContactListener::EndContact(b2Contact* contact) {
    for (Portal* portal : Portal::portals) {
        if (portal->midFixture == contact->GetFixtureA() || portal->collisionSensor == contact->GetFixtureA()) {
            portal->handleCollision(contact->GetFixtureB(), contact->GetFixtureA(), contact, END_CONTACT);
        }
        else if (portal->midFixture == contact->GetFixtureB() || portal->collisionSensor == contact->GetFixtureB()) {
            portal->handleCollision(contact->GetFixtureA(), contact->GetFixtureB(), contact, END_CONTACT);
        }
    }
}

void ContactListener::PreSolve(b2Contact* contact, const b2Manifold* oldManifold){
    for (Portal* p : Portal::portals) {
        p->handlePreCollision(contact->GetFixtureA(), contact->GetFixtureB(), contact, oldManifold);
    }

    for (Portal* p : Portal::portals) {
        p->handlePreCollision(contact->GetFixtureB(), contact->GetFixtureA(), contact, oldManifold);
    }
}

void ContactListener::PostSolve(b2Contact* contact, const b2ContactImpulse* impulse){

}