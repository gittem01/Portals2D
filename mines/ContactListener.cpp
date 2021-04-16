#include "ContactListener.h"
#include "Portal.h"

void ContactListener::BeginContact(b2Contact* contact){
    uintptr_t p1 = contact->GetFixtureA()->GetUserData().pointer;
    uintptr_t p2 = contact->GetFixtureB()->GetUserData().pointer;

    if (p1){
        Portal* portal = (Portal*)p1;
        if (portal->midFixture == contact->GetFixtureA()){
            portal->handleCollision(contact->GetFixtureB(), contact->GetFixtureA(), BEGIN_CONTACT);
        }
        else{
            portal->handleCollision(contact->GetFixtureA(), contact->GetFixtureB(), BEGIN_CONTACT);
        }
    }
    else if (p2){
        Portal* portal = (Portal*)p2;
        if (portal->midFixture == contact->GetFixtureA()){
            portal->handleCollision(contact->GetFixtureB(), contact->GetFixtureA(), BEGIN_CONTACT);
        }
        else{
            portal->handleCollision(contact->GetFixtureA(), contact->GetFixtureB(), BEGIN_CONTACT);
        }
    }
}

void ContactListener::EndContact(b2Contact* contact){
    uintptr_t p1 = contact->GetFixtureA()->GetUserData().pointer;
    uintptr_t p2 = contact->GetFixtureB()->GetUserData().pointer;

    if (p1){
        Portal* portal = (Portal*)p1;
        if (portal->portalBody == contact->GetFixtureA()->GetBody()){
            portal->handleCollision(contact->GetFixtureB(), contact->GetFixtureA(), END_CONTACT);
        }
        else{
            portal->handleCollision(contact->GetFixtureA(), contact->GetFixtureB(), END_CONTACT);
        }
    }
    else if (p2){
        Portal* portal = (Portal*)p2;
        if (portal->portalBody == contact->GetFixtureA()->GetBody()){
            portal->handleCollision(contact->GetFixtureB(), contact->GetFixtureA(), END_CONTACT);
        }
        else{
            portal->handleCollision(contact->GetFixtureA(), contact->GetFixtureB(), END_CONTACT);
        }
    }
}

void ContactListener::PreSolve(b2Contact* contact, const b2Manifold* oldManifold){
    printf("Presolve\n");
    uintptr_t p1 = contact->GetFixtureA()->GetBody()->GetUserData().pointer;
    uintptr_t p2 = contact->GetFixtureB()->GetBody()->GetUserData().pointer;

    if (p1){
        Portal* portal = (Portal*)p1;
        if (portal->collidingFixtures.find(contact->GetFixtureA()) != portal->collidingFixtures.end()){
            portal->handlePreCollision(contact->GetFixtureA(), contact, oldManifold);
        }
    }
    else if (p2){
        Portal* portal = (Portal*)p2;
        if (portal->collidingFixtures.find(contact->GetFixtureB()) != portal->collidingFixtures.end()){
            portal->handlePreCollision(contact->GetFixtureB(), contact, oldManifold);
        }
    }
}

void ContactListener::PostSolve(b2Contact* contact, const b2ContactImpulse* impulse){

}