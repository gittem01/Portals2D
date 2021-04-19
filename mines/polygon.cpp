#include "polygon.h"
#include <stdio.h>

b2Vec2 rotatePoint(b2Vec2 vec, float angle) {
    float x = cos(angle) * vec.x - sin(angle) * vec.y;
    float y = sin(angle) * vec.x + cos(angle) * vec.y;
    return b2Vec2(x, y);
}

polygon::polygon(b2World* world, b2Vec2 pos) {
    this->world = world;
    this->pos = pos;
}

void polygon::createShape(teleportData* data, b2BodyType bodyType) {
    b2BodyDef bodyDef;

    bodyDef.type = bodyType;
    bodyDef.linearDamping = 0.5f;
    body = world->CreateBody(&bodyDef);
    
    b2PolygonShape clone;
    clone = *(b2PolygonShape*)data->fixture->GetShape();
    b2Vec2* vertices = (b2Vec2*)malloc(clone.m_count * sizeof(b2Vec2));
    for (int i = 0; i < clone.m_count; i++) {
        b2Vec2 vertex = rotatePoint(clone.m_vertices[i], data->transform.q.GetAngle());
        vertices[i] = vertex;
    }

    clone.Set(vertices, clone.m_count);
    free(vertices);

    b2FixtureDef fixtureDef;
    fixtureDef.shape = &clone;
    fixtureDef.density = 1.0f;
    fixtureDef.restitution = 0.2f;
    fixtureDef.friction = 0.5f;
    body->CreateFixture(&fixtureDef);
}

void polygon::createBox(b2Vec2 size, b2BodyType bodyType) {
    b2BodyDef bodyDef;

    bodyDef.type = bodyType;
    bodyDef.position.Set(pos.x, pos.y);
    bodyDef.linearDamping = 0.5f;
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

void polygon::applyData() {
    this->createShape(data, b2_dynamicBody);
    body->SetTransform(data->transform.p, 0);
    body->SetLinearVelocity(data->linearVelocity);
    body->SetAngularVelocity(data->angularVelocity);
}