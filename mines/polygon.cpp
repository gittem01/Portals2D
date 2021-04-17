#include "polygon.h"
#include <stdio.h>

polygon::polygon(b2World* world, b2Vec2 pos) {
    this->world = world;
    this->pos = pos;
}

void polygon::createShape(b2PolygonShape shape, b2BodyType bodyType, void* userData) {
    b2BodyDef bodyDef;

    bodyDef.userData.pointer = (uintptr_t)userData;
    bodyDef.type = bodyType;
    bodyDef.linearDamping = 0.3f;
    body = world->CreateBody(&bodyDef);

    b2FixtureDef fixtureDef;
    fixtureDef.shape = &shape;
    fixtureDef.density = 1.0f;
    fixtureDef.restitution = 0.2f;
    fixtureDef.friction = 0.5f;
    body->CreateFixture(&fixtureDef);
}

void polygon::createBox(b2Vec2 size, b2BodyType bodyType, void* userData0) {
    b2BodyDef bodyDef;

    bodyDef.userData.pointer = (uintptr_t)userData0;
    bodyDef.type = bodyType;
    bodyDef.position.Set(pos.x, pos.y);
    bodyDef.linearDamping = 0.3f;
    body = world->CreateBody(&bodyDef);
    
    b2PolygonShape Box;
    Box.SetAsBox(size.x, size.y);
    b2FixtureDef fixtureDef;
    fixtureDef.shape = &Box;
    fixtureDef.density = 1.0f;
    fixtureDef.restitution = 0.2f;
    fixtureDef.friction = 0.5f;
    body->CreateFixture(&fixtureDef);
}

void polygon::setData(teleportData* data) {
    this->data = data;
}

void polygon::applyData(void* userData) {
    this->createShape(data->shape, b2_dynamicBody, userData);
    body->SetTransform(data->transform.p, data->transform.q.GetAngle());
    body->SetLinearVelocity(data->linearVelocity);
    body->SetAngularVelocity(data->angularVelocity);
    free(this->data);
}