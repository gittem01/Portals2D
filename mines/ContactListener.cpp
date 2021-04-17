#include "ContactListener.h"
#include "Portal.h"

void ContactListener::BeginContact(b2Contact* contact){
    uintptr_t p1 = contact->GetFixtureA()->GetUserData().pointer;
    uintptr_t p2 = contact->GetFixtureB()->GetUserData().pointer;

    for (Portal* portal : Portal::portals) {
        if (portal->midFixture == contact->GetFixtureA()) {
            portal->handleCollision(contact->GetFixtureB(), contact->GetFixtureA(), contact, BEGIN_CONTACT);
        }
        else if (portal->midFixture == contact->GetFixtureB()){
            portal->handleCollision(contact->GetFixtureA(), contact->GetFixtureB(), contact, BEGIN_CONTACT);
        }
    }
}

void ContactListener::EndContact(b2Contact* contact){
    uintptr_t p1 = contact->GetFixtureA()->GetUserData().pointer;
    uintptr_t p2 = contact->GetFixtureB()->GetUserData().pointer;

    for (Portal* portal : Portal::portals) {
        if (portal->midFixture == contact->GetFixtureA()) {
            portal->handleCollision(contact->GetFixtureB(), contact->GetFixtureA(), contact, END_CONTACT);
        }
        else if (portal->midFixture == contact->GetFixtureB()) {
            portal->handleCollision(contact->GetFixtureA(), contact->GetFixtureB(), contact, END_CONTACT);
        }
    }
}

void ContactListener::PreSolve(b2Contact* contact, const b2Manifold* oldManifold){
    uintptr_t p1 = contact->GetFixtureA()->GetBody()->GetUserData().pointer;
    uintptr_t p2 = contact->GetFixtureB()->GetBody()->GetUserData().pointer;

    if (p1){
        for (Portal* p : Portal::portals) {
            p->handlePreCollision(contact->GetFixtureA(), contact, oldManifold);
        }
        Portal* portal = (Portal*)p1;
        //portal->handlePreCollision(contact->GetFixtureA(), contact, oldManifold);
    }
    if (p2){
        Portal* portal = (Portal*)p2;
        for (Portal* p : Portal::portals) {
            p->handlePreCollision(contact->GetFixtureB(), contact, oldManifold);
        }
        //portal->handlePreCollision(contact->GetFixtureB(), contact, oldManifold);
    }
}

void ContactListener::PostSolve(b2Contact* contact, const b2ContactImpulse* impulse){

}