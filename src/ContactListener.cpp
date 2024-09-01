#include "ContactListener.h"
#include "mouseJointHandler.h"
#include <GameBase.h>
#include <Sounder.h>

// for mouse
void ContactListener::endBeginHandle(ContactType type, b2Contact* contact)
{
    bodyData* data1 = (bodyData*)(contact->GetFixtureA()->GetBody()->GetUserData().pointer);
    bodyData* data2 = (bodyData*)(contact->GetFixtureB()->GetBody()->GetUserData().pointer);
    if (data1)
    {
        switch (data1->type)
        {
        case B_MOUSE:
            if (contact->GetFixtureB()->GetBody()->GetType() != b2_staticBody)
            {
                mouseJointHandler* mjh = (mouseJointHandler*)(data1->data);
                if (type == BEGIN_CONTACT) mjh->collidingBodies.push_back(contact->GetFixtureB()->GetBody());
                else if (type == END_CONTACT)
                {
                    for (int i = 0; i < mjh->collidingBodies.size(); i++)
                    {
                        if (mjh->collidingBodies.at(i) == contact->GetFixtureB()->GetBody())
                        {
                            mjh->collidingBodies.erase(mjh->collidingBodies.begin() + i);
                        }
                    }
                }
            }
            break;
        default:
            break;
        }
    }
    if (data2)
    {
        switch (data2->type)
        {
        case B_MOUSE:
            if (contact->GetFixtureA()->GetBody()->GetType() != b2_staticBody)
            {
                mouseJointHandler* mjh = (mouseJointHandler*)(data2->data);
                if (type == BEGIN_CONTACT) mjh->collidingBodies.push_back(contact->GetFixtureA()->GetBody());
                else if (type == END_CONTACT)
                {
                    for (int i = 0; i < mjh->collidingBodies.size(); i++)
                    {
                        if (mjh->collidingBodies.at(i) == contact->GetFixtureA()->GetBody())
                        {
                            mjh->collidingBodies.erase(mjh->collidingBodies.begin() + i);
                        }
                    }
                }
            }
            break;
        default:
            break;
        }
    }
}

void ContactListener::BeginContact(b2Contact* contact)
{
    endBeginHandle(BEGIN_CONTACT, contact);

    bodyData* bDataA = (bodyData*)contact->GetFixtureA()->GetBody()->GetUserData().pointer;
    bodyData* bDataB = (bodyData*)contact->GetFixtureB()->GetBody()->GetUserData().pointer;

    if (bDataA && bDataA->type == B_PORTAL_BODY)
    {
        PortalBody* p = (PortalBody*)bDataA->data;
        p->collisionBegin(contact, contact->GetFixtureA(), contact->GetFixtureB());
    }

    if (bDataB && bDataB->type == B_PORTAL_BODY)
    {
        PortalBody* p = (PortalBody*)bDataB->data;
        p->collisionBegin(contact, contact->GetFixtureB(), contact->GetFixtureA());
    }

    if (!contact->IsEnabled())
    {
        return;
    }

    if (bDataA && bDataA->extraData)
    {
        g_PhysicsData* physicsData = (g_PhysicsData*)bDataA->extraData;
        if (physicsData->type & DEFAULT_OBJECT_O)
        {
            ((DefaultObject*)physicsData->data)->beginContact(contact, contact->GetFixtureB());
        }
    }
    if (bDataB && bDataB->extraData)
    {
        g_PhysicsData* physicsData = (g_PhysicsData*)bDataB->extraData;
        if (physicsData->type & DEFAULT_OBJECT_O)
        {
            ((DefaultObject*)physicsData->data)->beginContact(contact, contact->GetFixtureA());
        }
    }
}

void ContactListener::EndContact(b2Contact* contact)
{
    endBeginHandle(END_CONTACT, contact);

    bodyData* bDataA = (bodyData*)contact->GetFixtureA()->GetBody()->GetUserData().pointer;
    bodyData* bDataB = (bodyData*)contact->GetFixtureB()->GetBody()->GetUserData().pointer;

    if (bDataA && bDataA->type == B_PORTAL_BODY)
    {
        PortalBody* p = (PortalBody*)bDataA->data;
        p->collisionEnd(contact, contact->GetFixtureA(), contact->GetFixtureB());
    }

    if (bDataB && bDataB->type == B_PORTAL_BODY)
    {
        PortalBody* p = (PortalBody*)bDataB->data;
        p->collisionEnd(contact, contact->GetFixtureB(), contact->GetFixtureA());
    }

    if (!contact->IsEnabled())
    {
        return;
    }

    if (bDataA && bDataA->extraData)
    {
        g_PhysicsData* physicsData = (g_PhysicsData*)bDataA->extraData;
        if (physicsData->type & DEFAULT_OBJECT_O)
        {
            ((DefaultObject*)physicsData->data)->endContact(contact, contact->GetFixtureB());
        }
    }
    if (bDataB && bDataB->extraData)
    {
        g_PhysicsData* physicsData = (g_PhysicsData*)bDataB->extraData;
        if (physicsData->type & DEFAULT_OBJECT_O)
        {
            ((DefaultObject*)physicsData->data)->endContact(contact, contact->GetFixtureA());
        }
    }
}

void ContactListener::PreSolve(b2Contact* contact, const b2Manifold* oldManifold)
{
    bodyData* bDataA = (bodyData*)contact->GetFixtureA()->GetBody()->GetUserData().pointer;
    bodyData* bDataB = (bodyData*)contact->GetFixtureB()->GetBody()->GetUserData().pointer;

    if (bDataA && bDataA->type == B_PORTAL_BODY)
    {
        PortalBody* p = (PortalBody*)bDataA->data;
        p->preCollision(contact, contact->GetFixtureA(), contact->GetFixtureB());
    }
    if (bDataB && bDataB->type == B_PORTAL_BODY)
    {
        PortalBody* p = (PortalBody*)bDataB->data;
        p->preCollision(contact, contact->GetFixtureB(), contact->GetFixtureA());
    }

    if (!contact->IsEnabled())
    {
        return;
    }

    if (bDataA && bDataA->extraData)
    {
        g_PhysicsData* physicsData = (g_PhysicsData*)bDataA->extraData;
        if (physicsData->type & DEFAULT_OBJECT_O)
        {
            ((DefaultObject*)physicsData->data)->preSolve(contact, contact->GetFixtureA()->GetBody(), contact->GetFixtureB());
        }
    }
    if (bDataB && bDataB->extraData)
    {
        g_PhysicsData* physicsData = (g_PhysicsData*)bDataB->extraData;
        if (physicsData->type & DEFAULT_OBJECT_O)
        {
            ((DefaultObject*)physicsData->data)->preSolve(contact, contact->GetFixtureB()->GetBody(), contact->GetFixtureA());
        }
    }
}

void ContactListener::PostSolve(b2Contact* contact, const b2ContactImpulse* impulse)
{
    if (!contact->IsEnabled())
    {
        return;
    }

    bodyData* bDataA = (bodyData*)contact->GetFixtureA()->GetBody()->GetUserData().pointer;
    bodyData* bDataB = (bodyData*)contact->GetFixtureB()->GetBody()->GetUserData().pointer;
    
    if (bDataA && bDataA->extraData)
    {
        g_PhysicsData* physicsData = (g_PhysicsData*)bDataA->extraData;
        if (physicsData->type & DEFAULT_OBJECT_O)
        {
            ((DefaultObject*)physicsData->data)->reportContact(contact, impulse, contact->GetFixtureA()->GetBody(), contact->GetFixtureB());
        }
    }
    if (bDataB && bDataB->extraData)
    {
        g_PhysicsData* physicsData = (g_PhysicsData*)bDataB->extraData;
        if (physicsData->type & DEFAULT_OBJECT_O)
        {
            ((DefaultObject*)physicsData->data)->reportContact(contact, impulse, contact->GetFixtureB()->GetBody(), contact->GetFixtureA());
        }
    }
}
