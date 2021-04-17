#include "Portal.h"
#include <math.h>
#include <stdio.h>
#include <GLFW/glfw3.h>


bool isLeft(b2Vec2 a, b2Vec2 b, b2Vec2 c){
     return ((b.x - a.x)*(c.y - a.y) - (b.y - a.y)*(c.x - a.x)) >= 0.03f;
}

float calcAngle(b2Vec2 vec){
    float angle = atan2(vec.y, vec.x);
    //if (angle < 0) angle += M_PI * 2.0f;

    return angle;
}

float vecAngle(b2Vec2 v1, b2Vec2 v2){
    return acos(b2Dot(v1, v2));
}

void rotateVec(b2Vec2* vec, float angle){
    float x = cos(angle) * vec->x - sin(angle) * vec->y;
    float y = sin(angle) * vec->x + cos(angle) * vec->y;
    vec->x = x;
    vec->y = y;
}

void normalize(b2Vec2* vec){
    float length = sqrt(vec->x * vec->x + vec->y * vec->y);
    if (length == 0) { return; }
    
    vec->x /= length;
    vec->y /= length;
}


Portal::Portal(b2Vec2 pos, b2Vec2 dir, float size, b2World* world){
    this->pos = pos;
    this->dir = dir;
    normalize(&dir);
    this->size = size;
    calculatePoints();
    createPhysicalBody(world);
    
    this->connectedPortal = NULL;
    this->color = b2Color(0.0f, 0.3f, 1.0f, 1.0f);
}


void Portal::calculatePoints(){
    this->angle = calcAngle(this->dir) + b2_pi / 2.0f;

    points[0].x = pos.x + cos(angle) * size;
    points[0].y = pos.y + sin(angle) * size;

    points[1].x = pos.x - cos(angle) * size;
    points[1].y = pos.y - sin(angle) * size;
}

void Portal::createPhysicalBody(b2World* world){
    b2BodyDef bd;
    bd.type = b2_staticBody;
    portalBody = world->CreateBody(&bd);

    b2EdgeShape shape;
    shape.SetTwoSided(points[0], points[1]);

    b2FixtureDef midPortal;
    midPortal.shape = &shape;
    midPortal.userData.pointer = (uintptr_t)this;

    midFixture = portalBody->CreateFixture(&midPortal);
    midFixture->SetSensor(true);
    //printf("Here: %p\n", midFixture);

    b2Vec2 pVec = points[1] - points[0];
    normalize(&pVec);
    pVec = b2Vec2(pVec.x * 0.1f, pVec.y * 0.1f);

    shape.SetTwoSided(points[0] - pVec, points[0]);
    yFix[0] = portalBody->CreateFixture(&shape, 0.0f);
    
    shape.SetTwoSided(points[1] + pVec, points[1]);
    yFix[1] = portalBody->CreateFixture(&shape, 0.0f);
}

void Portal::handleCollision(b2Fixture* fix1, b2Fixture* fix2, b2Contact* contact, contactType type){
    if (type == BEGIN_CONTACT) {
        collidingFixtures.insert(fix1);
        if (unhandledCollisions.find(fix1) != unhandledCollisions.end()) {
            //printf("Hereeeeeeee\n");
            for (b2Contact* contact0 : unhandledCollisions[fix1]) {
                handlePreCollision(fix1, contact0, NULL);
            }
        }

        if (connectedPortal && 
            connectedPortal->collidingFixtures.find(fix1) == connectedPortal->collidingFixtures.end()) {
            b2Body* body = fix1->GetBody();
            b2World* world = body->GetWorld();
            float angle = vecAngle(this->dir, connectedPortal->dir);
            float angularVelocity = body->GetAngularVelocity();
            b2Vec2 posDiff = body->GetPosition() - this->pos;
            b2Vec2 linearVelocity = body->GetLinearVelocity();
            rotateVec(&linearVelocity, angle);
            rotateVec(&posDiff, -angle);

            polygon* poly = new polygon(world, connectedPortal->pos + posDiff);
            b2Transform transform;
            transform.p = poly->pos;
            transform.q.Set(angle + body->GetAngle());
            teleportData* data = (teleportData*)malloc(sizeof(teleportData));
            *data = { transform, linearVelocity, angularVelocity, *(b2PolygonShape*)fix1->GetShape() };
            poly->setData(data);
            addPolygons.push_back(poly);
        }

        //printf("Begin\n");
    }
    else if (type == END_CONTACT) {
        if (isLeft(points[0], points[1], fix1->GetBody()->GetPosition())){
            // Alive
        }
        else{
            destroyQueue.insert(fix1->GetBody());
        }
        collidingFixtures.erase(fix1);
        //printf("End\n");
    }
}

void Portal::handlePreCollision(b2Fixture* fixture, b2Contact* contact, const b2Manifold* oldManifold){
    if (unhandledCollisions.find(fixture) == unhandledCollisions.end()) {
        unhandledCollisions[fixture] = std::set<b2Contact*>();
    }
    unhandledCollisions[fixture].insert(contact);

    if (this->collidingFixtures.find(fixture) != this->collidingFixtures.end() ||
        this->destroyQueue.find(fixture->GetBody()) != this->destroyQueue.end()) {

    }
    else {
        return;
    }

    b2Fixture* otherFixture;
    if (contact->GetFixtureA()->GetBody() != fixture->GetBody()){
        otherFixture = contact->GetFixtureA();
    }
    else{
        otherFixture = contact->GetFixtureB();
    }

    b2World* world = fixture->GetBody()->GetWorld();

    b2WorldManifold wManifold;
    contact->GetWorldManifold(&wManifold);

    bool isIn = false;
    if (otherFixture->GetType() == b2Shape::e_polygon){
        for (int i=0; i<contact->GetManifold()->pointCount; i++){
            if (otherFixture->TestPoint(wManifold.points[i])){
                isIn = true;
            }
        }
    }

    if (otherFixture->GetShape()->GetType() == b2Shape::e_edge && otherFixture != midFixture){
        isIn = true;
    }

    bool collide = shouldCollide(wManifold, contact->GetManifold()->pointCount);
    if ((!collide || !isIn || vecAngle(wManifold.normal, dir) < 0.0f) 
        && otherFixture != yFix[0] && otherFixture != yFix[1]){
        contact->SetEnabled(false);
    }

    for (int i=0; i<contact->GetManifold()->pointCount; i++){
        //world->m_debugDraw->DrawPoint(wManifold.points[i], 6.0f, b2Color(1, 1, 1, 1));
    }
}

bool Portal::shouldCollide(b2WorldManifold wManifold, int numOfPoints){
    bool in = true;
    for (int i=0; i<numOfPoints; i++){
        if (!isLeft(points[0], points[1], wManifold.points[i])){
            in = false;
        }
    }

    return in;
}

void Portal::update(){
    for (b2Body* destroyBody: destroyQueue){
        destroyBody->GetWorld()->DestroyBody(destroyBody);
    }
    for (polygon* poly : addPolygons) {
        //printf("Herexx\n");
        poly->applyData(connectedPortal);
    }
    addPolygons.clear();
    unhandledCollisions.clear();
    destroyQueue.clear();
}

void Portal::draw(){
    glLineWidth(2.0f);
	glColor4f(color.r, color.g, color.b, color.a);
	glBegin(GL_LINES);
	glVertex2d(points[0].x, points[0].y);
	glVertex2d(points[1].x, points[1].y);
	glEnd();
}

void Portal::connect(Portal* portal2){
    connectedPortal = portal2;
    portal2->connectedPortal = this;
}

void Portal::onContact(b2Body* body){
    if (!connectedPortal) return;
    connectedPortal->newBody(body);
}

void Portal::newBody(b2Body* body){
    
}