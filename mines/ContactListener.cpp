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
                if (type == BEGIN_CONTACT) mjh->collidingBodies.push_back(contact->GetFixtureB()->GetBody());
                else if (type == END_CONTACT){
                    for (int i = 0; i < mjh->collidingBodies.size(); i++){
                        if (mjh->collidingBodies.at(i) == contact->GetFixtureB()->GetBody()){
                            mjh->collidingBodies.erase(mjh->collidingBodies.begin() + i);
                        }
                    }
                }
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
                if (type == BEGIN_CONTACT) mjh->collidingBodies.push_back(contact->GetFixtureA()->GetBody());
                else if (type == END_CONTACT){
                    for (int i = 0; i < mjh->collidingBodies.size(); i++){
                        if (mjh->collidingBodies.at(i) == contact->GetFixtureA()->GetBody()){
                            mjh->collidingBodies.erase(mjh->collidingBodies.begin() + i);
                        }
                    }
                }
            }
            break;
        }
    }
}

void ContactListener::BeginContact(b2Contact* contact){
    
    endBeginHandle(BEGIN_CONTACT, contact);
}

void ContactListener::EndContact(b2Contact* contact) {
    
    endBeginHandle(END_CONTACT, contact);
}

void ContactListener::PreSolve(b2Contact* contact, const b2Manifold* oldManifold){
    
}

void ContactListener::PostSolve(b2Contact* contact, const b2ContactImpulse* impulse){

}