#include "Shape.h"

b2Vec2 rotatePoint(b2Vec2 vec, float angle) {
    float x = cos(angle) * vec.x - sin(angle) * vec.y;
    float y = sin(angle) * vec.x + cos(angle) * vec.y;
    return b2Vec2(x, y);
}

Shape::Shape(b2World* world, b2Vec2 pos) {
    this->world = world;
    this->pos = pos;
}

void Shape::createPolyFromData(teleportData* data) {
    b2BodyDef bodyDef;

    bodyDef.type = b2_dynamicBody;
    bodyDef.linearDamping = defaultLinearDamping;
    bodyDef.angularDamping = defaultAngularDamping;
    body = world->CreateBody(&bodyDef);

    b2PolygonShape clone;
    clone = *(b2PolygonShape*)data->fixture->GetShape();
    b2Vec2* vertices = (b2Vec2*)malloc(clone.m_count * sizeof(b2Vec2));
    for (int i = 0; i < clone.m_count; i++) {
        b2Vec2 vertex = rotatePoint(clone.m_vertices[i], data->angle + data->fixture->GetBody()->GetAngle());
        vertices[i] = vertex;
    }

    clone.Set(vertices, clone.m_count);
    free(vertices);

    b2FixtureDef fixtureDef;
    fixtureDef.shape = &clone;
    fixtureDef.density = defaultDensity;
    fixtureDef.restitution = defaultrestitution;
    fixtureDef.friction = defaultFriction;
    body->CreateFixture(&fixtureDef);
    body->SetBullet(isBullet);
}

void Shape::createCircleFromData(teleportData* data) {
    b2BodyDef bodyDef;

    bodyDef.type = b2_dynamicBody;
    bodyDef.linearDamping = defaultLinearDamping;
    bodyDef.angularDamping = defaultAngularDamping;
    body = world->CreateBody(&bodyDef);

    b2CircleShape clone;
    clone = *(b2CircleShape*)data->fixture->GetShape();

    b2FixtureDef fixtureDef;
    fixtureDef.shape = &clone;
    fixtureDef.density = defaultDensity;
    fixtureDef.restitution = defaultrestitution;
    fixtureDef.friction = defaultFriction;

    body->CreateFixture(&fixtureDef);
    body->SetBullet(isBullet);
}

void Shape::createRect(b2Vec2 size, b2BodyType bodyType) {
    b2BodyDef bodyDef;

    bodyDef.type = bodyType;
    bodyDef.position.Set(pos.x, pos.y);
    bodyDef.linearDamping = defaultLinearDamping;
    bodyDef.angularDamping = defaultAngularDamping;
    body = world->CreateBody(&bodyDef);

    b2PolygonShape Box;
    Box.SetAsBox(size.x, size.y);
    b2FixtureDef fixtureDef;
    fixtureDef.shape = &Box;
    fixtureDef.density = defaultDensity;
    fixtureDef.restitution = defaultrestitution;
    fixtureDef.friction = defaultFriction;
    body->CreateFixture(&fixtureDef);
    body->SetBullet(isBullet);
}

void Shape::createCircle(float r, b2BodyType bodyType) {
    b2BodyDef bodyDef;

    bodyDef.type = bodyType;
    bodyDef.position.Set(pos.x, pos.y);
    bodyDef.linearDamping = defaultLinearDamping;
    bodyDef.angularDamping = defaultAngularDamping;
    body = world->CreateBody(&bodyDef);

    b2CircleShape circle;
    circle.m_radius = r;

    b2FixtureDef fixtureDef;
    fixtureDef.shape = &circle;
    fixtureDef.density = defaultDensity;
    fixtureDef.restitution = defaultrestitution;
    fixtureDef.friction = defaultFriction;
    body->CreateFixture(&fixtureDef);
    body->SetBullet(isBullet);
}

void Shape::setData(teleportData* data) {
    this->data = data;
}

void Shape::applyData() {
    if (data->fixture->GetShape()->GetType() == b2Shape::e_polygon) {
        this->createPolyFromData(data);
    }
    else if (data->fixture->GetShape()->GetType() == b2Shape::e_circle) {
        this->createCircleFromData(data);
    }

    float angularVelocity = data->fixture->GetBody()->GetAngularVelocity();
    b2Vec2 posDiff = data->p1 - data->fixture->GetBody()->GetPosition();

    b2Vec2 linearVelocity = data->fixture->GetBody()->GetLinearVelocity();
    linearVelocity = rotatePoint(linearVelocity, data->angle);
    posDiff = rotatePoint(posDiff, b2_pi + data->angle);

    b2Transform transform;
    transform.p = data->p2 + posDiff;
    this->pos = transform.p;
    transform.q.Set(data->angle + data->fixture->GetBody()->GetAngle());

    if (data->fixture->GetShape()->GetType() == b2Shape::e_polygon) {
        body->SetTransform(transform.p, 0);
    }
    else if (data->fixture->GetShape()->GetType() == b2Shape::e_circle) {
        body->SetTransform(transform.p, transform.q.GetAngle());
    }

    body->SetLinearVelocity(linearVelocity);
    body->SetAngularVelocity(angularVelocity);
}