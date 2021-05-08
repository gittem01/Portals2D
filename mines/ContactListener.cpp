#include "ContactListener.h"
#include "mouseJointHandler.h"

void ContactListener::endBeginHandle(contactType type, b2Contact* contact) {
    bodyData* data1 = (bodyData*)(contact->GetFixtureA()->GetBody()->GetUserData().pointer);
    bodyData* data2 = (bodyData*)(contact->GetFixtureB()->GetBody()->GetUserData().pointer);
    if (data1) {
        switch (data1->type)
        {
        case MOUSE:
            if (contact->GetFixtureB()->GetBody()->GetType() != b2_staticBody) {
                mouseJointHandler* mjh = (mouseJointHandler*)(data1->data);
                if (type == BEGIN_CONTACT) mjh->collidingBodies.insert(contact->GetFixtureB()->GetBody());
                else if (type == END_CONTACT) mjh->collidingBodies.erase(contact->GetFixtureB()->GetBody());
            }
            break;
        }
    }
    if (data2) {
        switch (data2->type)
        {
        case MOUSE:
            if (contact->GetFixtureA()->GetBody()->GetType() != b2_staticBody) {
                mouseJointHandler* mjh = (mouseJointHandler*)(data2->data);
                if (type == BEGIN_CONTACT) mjh->collidingBodies.insert(contact->GetFixtureA()->GetBody());
                else if (type == END_CONTACT) mjh->collidingBodies.erase(contact->GetFixtureA()->GetBody());
            }
            break;
        }
    }
}

void ContactListener::BeginContact(b2Contact* contact){
    for (Portal* portal : Portal::portals) {
        if (portal->midFixture == contact->GetFixtureA() || portal->collisionSensor == contact->GetFixtureA()) {
            portal->handleCollision(contact->GetFixtureB(), contact->GetFixtureA(), contact, BEGIN_CONTACT);
        }
        else if (portal->midFixture == contact->GetFixtureB() || portal->collisionSensor == contact->GetFixtureB()) {
            portal->handleCollision(contact->GetFixtureA(), contact->GetFixtureB(), contact, BEGIN_CONTACT);
        }
    }
    endBeginHandle(BEGIN_CONTACT, contact);
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
    endBeginHandle(END_CONTACT, contact);
}

void ContactListener::PreSolve(b2Contact* contact, const b2Manifold* oldManifold){
    bool handled = false;
    for (Portal* p : Portal::portals) {
        handled = p->handlePreCollision(contact->GetFixtureA(), contact->GetFixtureB(), contact, oldManifold);
        if (!handled) {
            handled = p->handlePreCollision(contact->GetFixtureB(), contact->GetFixtureA(), contact, oldManifold);
        }
        if (handled) break;
    }
}

void ContactListener::PostSolve(b2Contact* contact, const b2ContactImpulse* impulse){

}