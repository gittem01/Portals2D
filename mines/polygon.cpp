#include "polygon.h"

polygon::polygon(b2World* world, b2Vec2 pos) {
    this->world = world;
    this->pos = pos;
}

void polygon::createBox(b2Vec2 size, b2BodyType bodyType, void* userData0) {
    b2BodyDef bodyDef;

    bodyDef.userData.pointer = (uintptr_t)userData0;
    bodyDef.type = bodyType;
    bodyDef.position.Set(pos.x, pos.y);
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