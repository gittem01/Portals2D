#include "Portal.h"

b2Vec2 rotatePoint(b2Vec2 vec, float angle) {
    float x = cos(angle) * vec.x - sin(angle) * vec.y;
    float y = sin(angle) * vec.x + cos(angle) * vec.y;
    return b2Vec2(x, y);
}

float calcAngle(b2Vec2 vec) {
    float angle = atan2(vec.y, vec.x);
    if (angle < 0) angle += b2_pi * 2.0f;

    return angle;
}

Shape::Shape(b2World* world, b2Vec2 pos) {
    this->world = world;
    this->pos = pos;
    this->body = NULL;
    this->data = NULL;
}

Shape::~Shape() {
    
}

void Shape::portalCollideStart(void* portal, b2Fixture* fix) {
    Portal* tPortal = reinterpret_cast<Portal*>(portal);

    float angle0 = -calcAngle(tPortal->dir) + calcAngle(-tPortal->connectedPortal->dir);

    data = (teleportData*)malloc(sizeof(teleportData));
    *data = { tPortal->pos, tPortal->connectedPortal->pos, angle0, fix };

    setData(data);
}

void Shape::portalCollideEnd(void* portal, b2Fixture* fix, bool shouldDestroy) {
    Portal* tPortal = reinterpret_cast<Portal*>(portal);

    if (shouldDestroy) {
        dtrBodies.insert(fix->GetBody());
    }
    else {
        dtrBodies.insert(tPortal->correspondingBodies[fix->GetBody()]);
    }
}

void Shape::creation() {
    applyData();
}

void Shape::destruction() {
    for (b2Body* destroyBody : dtrBodies) {
        free(((bodyData*)destroyBody->GetUserData().pointer));
        world->DestroyBody(destroyBody);

        int i = 0;
        for (b2Body* body : bodies) {
            if (body == destroyBody) {
                bodies.erase(bodies.begin() + i++);
                break;
            }
        }
    }
    dtrBodies.clear();
}

void Shape::createPolyFromData(teleportData* data) {
    b2BodyDef bodyDef;

    bodyDef.type = b2_dynamicBody;
    bodyDef.linearDamping = data->fixture->GetBody()->GetLinearDamping();
    bodyDef.angularDamping = data->fixture->GetBody()->GetAngularDamping();
    bodyData* bData = (bodyData*)malloc(sizeof(bodyData));
    *bData = { SHAPE, this };
    bodyDef.userData.pointer = (uintptr_t)bData;

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
    fixtureDef.density = data->fixture->GetDensity();
    fixtureDef.restitution = data->fixture->GetRestitution();
    fixtureDef.friction = data->fixture->GetFriction();

    body->CreateFixture(&fixtureDef);
    body->SetBullet(isBullet);

    bodies.push_back(body);
}

void Shape::createCircleFromData(teleportData* data) {
    b2BodyDef bodyDef;

    bodyDef.type = b2_dynamicBody;
    bodyDef.linearDamping = data->fixture->GetBody()->GetLinearDamping();
    bodyDef.angularDamping = data->fixture->GetBody()->GetAngularDamping();
    bodyData* bData = (bodyData*)malloc(sizeof(bodyData));
    *bData = { SHAPE, this };
    bodyDef.userData.pointer = (uintptr_t)bData;

    body = world->CreateBody(&bodyDef);

    b2CircleShape clone;
    clone = *(b2CircleShape*)data->fixture->GetShape();

    b2FixtureDef fixtureDef;
    fixtureDef.shape = &clone;
    fixtureDef.density = data->fixture->GetDensity();
    fixtureDef.restitution = data->fixture->GetRestitution();
    fixtureDef.friction = data->fixture->GetFriction();

    body->CreateFixture(&fixtureDef);
    body->SetBullet(isBullet);

    bodies.push_back(body);
}

void Shape::createRect(b2Vec2 size, b2BodyType bodyType) {
    b2BodyDef bodyDef;

    bodyDef.type = bodyType;
    bodyDef.position.Set(pos.x, pos.y);
    bodyDef.linearDamping = defaultLinearDamping;
    bodyDef.angularDamping = defaultAngularDamping;
    bodyData* bData = (bodyData*)malloc(sizeof(bodyData));
    *bData = { SHAPE, this };
    bodyDef.userData.pointer = (uintptr_t)bData;
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

    bodies.push_back(body);
}

void Shape::createCircle(float r, b2BodyType bodyType) {
    b2BodyDef bodyDef;

    bodyDef.type = bodyType;
    bodyDef.position.Set(pos.x, pos.y);
    bodyDef.linearDamping = defaultLinearDamping;
    bodyDef.angularDamping = defaultAngularDamping;
    bodyData* bData = (bodyData*)malloc(sizeof(bodyData));
    *bData = { SHAPE, this };
    bodyDef.userData.pointer = (uintptr_t)bData;
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

    bodies.push_back(body);
}

void Shape::setData(teleportData* data) {
    this->data = data;
}

void Shape::applyData() {
    if (data->fixture->GetShape()->GetType() == b2Shape::e_polygon) {
        createPolyFromData(data);
    }
    else if (data->fixture->GetShape()->GetType() == b2Shape::e_circle) {
        createCircleFromData(data);
    }

    float angularVelocity = data->fixture->GetBody()->GetAngularVelocity();
    b2Vec2 posDiff = data->p1 - data->fixture->GetBody()->GetPosition();

    b2Vec2 linearVelocity = data->fixture->GetBody()->GetLinearVelocity();
    linearVelocity = rotatePoint(linearVelocity, data->angle);
    posDiff = rotatePoint(posDiff, b2_pi + data->angle);

    b2Transform transform;
    transform.p = data->p2 + posDiff;
    pos = transform.p;
    transform.q.Set(data->angle + data->fixture->GetBody()->GetAngle());

    if (data->fixture->GetShape()->GetType() == b2Shape::e_polygon) {
        body->SetTransform(transform.p, 0);
    }
    else if (data->fixture->GetShape()->GetType() == b2Shape::e_circle) {
        body->SetTransform(transform.p, transform.q.GetAngle());
    }

    body->SetLinearVelocity(linearVelocity);
    body->SetAngularVelocity(angularVelocity);

    free(data);
}