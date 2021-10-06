#include "Portal.h"
#include <math.h>
#include <stdio.h>
#include <chrono>
#include <thread>
#include <GLFW/glfw3.h>

std::set<Portal*> Portal::portals;

bool isLeft(b2Vec2& a, b2Vec2& b, b2Vec2& c, float t){
     return ((b.x - a.x)*(c.y - a.y) - (b.y - a.y)*(c.x - a.x)) >= t; // check this
}

float calcAngle(b2Vec2 vec) {
    float angle = atan2(vec.y, vec.x);
    if (angle < 0) angle += b2_pi * 2.0f;

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
    this->world = world;
    this->pos = pos;
    this->dir = dir;
    normalize(&this->dir);
    this->size = size;
    calculatePoints();
    createPhysicalBody(world);
    
    this->connectedPortal = NULL;
    this->color = b2Color(0.0f, 0.3f, 1.0f, 1.0f);

    portals.insert(this);
}

Portal::~Portal() {
    world->DestroyBody(portalBody);
    portals.erase(this);
    if (connectedPortal) connectedPortal->clear();
    this->clear();
}

void Portal::clear() {
    connectedPortal = NULL;
    collidingFixtures.clear();
    destroyQueue.clear();
    prepareFixtures.clear();
    addShapes.clear();
    addBodies.clear();
    correspondingBodies.clear();
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

    float d = 0.05f;
    b2Vec2 smallDir = b2Vec2(dir.x * d, dir.y * d);
    b2EdgeShape shape;

    shape.SetTwoSided(points[0] - smallDir, points[1] - smallDir);

    b2FixtureDef midPortal;
    midPortal.shape = &shape;

    bodyData* data = (bodyData*)malloc(sizeof(bodyData));
    *data = { PORTAL, this };
    midPortal.userData.pointer = (uintptr_t)&data;

    midFixture = portalBody->CreateFixture(&midPortal);

    shape.SetTwoSided(points[0], points[0]);
    yFix[0] = portalBody->CreateFixture(&shape, 0.0f);
    
    shape.SetTwoSided(points[1], points[1]);
    yFix[1] = portalBody->CreateFixture(&shape, 0.0f);

    b2Vec2 pVec = points[1] - points[0];
    normalize(&pVec);
    pVec = b2Vec2(pVec.x * 0.1f, pVec.y * 0.1f);

    float widthMul = 10.0f;
    float dirMult = 2.0f;
    b2Vec2 p1 = points[0] - b2Vec2(pVec.x * widthMul, pVec.y * widthMul);
    b2Vec2 p2 = points[1] + b2Vec2(pVec.x * widthMul, pVec.y * widthMul);
    b2Vec2 p3 = p1 + b2Vec2(dir.x * dirMult, dir.y * dirMult);
    b2Vec2 p4 = p2 + b2Vec2(dir.x * dirMult, dir.y * dirMult);
    
    b2Vec2 polyPoints[4] = { p1, p2, p3, p4 };

    b2PolygonShape polyShape;
    polyShape.Set(polyPoints, 4);
    collisionSensor = portalBody->CreateFixture(&polyShape, 0.0f);
    collisionSensor->SetSensor(true);

    b2Vec2 polyPoints2[4] = { p1 + smallDir, p2 + smallDir, p1-dir, p2-dir };
    bottomShape.Set(polyPoints2, 4);
}

void Portal::handleCollision(b2Fixture* fix1, b2Fixture* fix2, b2Contact* contact, contactType type){
    if (!this->connectedPortal) return;

    b2Vec2 fix1Pos = fix1->GetBody()->GetPosition();

    if (type == BEGIN_CONTACT) {

        if (fix2 == collisionSensor) {
            prepareFixtures.insert(fix1);
            return;
        }

        if (collidingFixtures.find(fix1) != collidingFixtures.end()) {
            return;
        }

        if (!isLeft(points[0], points[1], fix1Pos, 0.0f)) {
            return;
        }

        if (connectedPortal && collidingFixtures.find(fix1) == collidingFixtures.end() &&
            destroyQueue.find(fix1->GetBody()) == destroyQueue.end()) {

            b2World* world = fix1->GetBody()->GetWorld();

            float angle0 = -calcAngle(this->dir) + calcAngle(-connectedPortal->dir);

            Shape* shape = new Shape(world, b2Vec2());

            teleportData* data = (teleportData*)malloc(sizeof(teleportData));
            *data = { this->pos, connectedPortal->pos, angle0, fix1 };

            shape->setData(data);
            addShapes.push_back(shape);
            addBodies.push_back(fix1->GetBody());
        }
        collidingFixtures.insert(fix1);
    }
    else if (type == END_CONTACT) {
        if (fix2 == collisionSensor) {
            prepareFixtures.erase(fix1);
            return;
        }
        if (collidingFixtures.find(fix1) == collidingFixtures.end()) return;
        if (isLeft(points[0], points[1], fix1Pos, 0.1f)){
            if (correspondingBodies.find(fix1->GetBody()) != correspondingBodies.end()) {
                b2Body* cBody = correspondingBodies[fix1->GetBody()];
                connectedPortal->destroyQueue.insert(cBody);
                connectedPortal->collidingFixtures.erase(cBody->GetFixtureList());
                connectedPortal->correspondingBodies.erase(cBody);
                connectedPortal->prepareFixtures.erase(cBody->GetFixtureList());
            }
        }
        else{
            if (correspondingBodies.find(fix1->GetBody()) != correspondingBodies.end()) {
                connectedPortal->correspondingBodies.erase(correspondingBodies[fix1->GetBody()]);
                correspondingBodies.erase(fix1->GetBody());
            }
            destroyQueue.insert(fix1->GetBody());
        }
        collidingFixtures.erase(fix1);
        correspondingBodies.erase(fix1->GetBody());
    }
}

bool Portal::handlePreCollision(b2Fixture* fixture, b2Fixture* otherFixture, 
    b2Contact* contact, const b2Manifold* oldManifold){

    int mode;
    if (this->collidingFixtures.find(fixture) != this->collidingFixtures.end() ||
        this->destroyQueue.find(fixture->GetBody()) != this->destroyQueue.end()) {
        mode = 1;
    }
    else if (prepareFixtures.find(fixture) != prepareFixtures.end() && otherFixture->GetType() != b2_dynamicBody) {
        mode = 2;
    }
    else {
        return false;
    }

    if (correspondingBodies.find(otherFixture->GetBody()) != correspondingBodies.end() ||
        fixture->GetBody()->GetType() == b2_staticBody) {
        return false;
    }

    b2World* world = fixture->GetBody()->GetWorld();

    b2WorldManifold wManifold;
    contact->GetWorldManifold(&wManifold);

    std::vector<b2Vec2> collisionPoints = getCollisionPoints(fixture, otherFixture);

    bool noCollision = false;
    if (otherFixture->GetShape()->GetType() != b2Shape::e_edge) {
        for (b2Vec2& p : collisionPoints) {
            if (!isLeft(points[0], points[1], p, 0.0f)) {
                noCollision = true;
            }
            else {
                noCollision = false;
                break;
            }
        }
    }
    
    for (int i = 0; i < collisionPoints.size(); i++) {
        world->m_debugDraw->DrawPoint(collisionPoints.at(i), 10.0f, b2Color(1, 1, 1, 1));
    }

    b2Vec2* finalPos = (b2Vec2*)malloc(contact->GetManifold()->pointCount * sizeof(b2Vec2));

    b2Vec2 normalMult = b2Vec2(wManifold.normal.x * 100.0f, wManifold.normal.y * 100.0f);
    for (int i = 0; i < contact->GetManifold()->pointCount; i++) {
        finalPos[i] = wManifold.points[i];

        b2RayCastOutput rcOutput;
        b2RayCastInput rcInput;

        rcInput.maxFraction = 1.0f;
        rcInput.p1 = wManifold.points[i] + normalMult;
        rcInput.p2 = wManifold.points[i] - normalMult;

        bool success;
        success = otherFixture->RayCast(&rcOutput, rcInput, otherFixture->GetBody()->GetType());
            
        if (success) {
            finalPos[i] = getRayPoint(rcInput, rcOutput);
        }
        else if (otherFixture->GetShape()->GetType() == b2Shape::e_edge) {
            if (collisionPoints.size() > i) {
                finalPos[i] = collisionPoints.at(i);
            }
        }
    }

    for (int i = 0; i < contact->GetManifold()->pointCount; i++) {
        // Contact drawing
        //world->m_debugDraw->DrawPoint(finalPos[i], 10.0f, b2Color(1, 0, 0, 1));
    }

    bool collide = shouldCollide(finalPos, contact->GetManifold(), mode);
    free(finalPos);
    if ((!collide || noCollision) && otherFixture != yFix[0] && otherFixture != yFix[1]) {
        contact->SetEnabled(false);
        return true;
    }
    
    return false;
}

bool Portal::shouldCollide(b2Vec2* finalPos, b2Manifold* manifold, int mode){
    int numOfPoints = manifold->pointCount;
    bool in = true;
    bool others[2] = { false, false };
    if (mode == 1) {
        for (int i = 0; i < numOfPoints; i++) {
            if (!isLeft(points[0], points[1], finalPos[i], 0.001f)) {
                in = false;
            }
            else if (numOfPoints == 2) {
                others[i] = true;
            }
        }
    }
    else if (mode == 2) {
        for (int i = 0; i < numOfPoints; i++) {
            bool a1 = isLeft(points[0], points[1], finalPos[i], 0.001f);
            bool a2 = isLeft(points[0] + dir, points[0], finalPos[i], 0.0f);
            bool a3 = isLeft(points[1] + dir, points[1], finalPos[i], 0.0f);
            if (!a1 && a2 && !a3) {
                in = false;
            }
            else if (numOfPoints == 2) {
                others[i] = true;
            }
        }
    }

    if (numOfPoints == 2 && (others[0] ^ others[1])) {
        int n = 0;
        if (others[1]) n = 1;
        manifold->pointCount -= 1;
        manifold->points[0] = manifold->points[n];
        in = true;
    }

    return in;
}

void Portal::connectBodies(b2Body* body1, b2Body* body2) {
    b2PrismaticJointDef prismDef;
    prismDef.Initialize(body1, body2, b2Vec2(0.0f, 0.0f), b2Vec2(0.0f, 0.0f));
    prismDef.collideConnected = true;
    prismDef.maxMotorForce = 0.0f;

    body1->GetWorld()->CreateJoint(&prismDef);

    b2Vec2 dirClone1 = connectedPortal->dir;
    b2Vec2 dirClone2 = this->dir;
    // will be replaced later (no guarantee). Dynamic positioning will be applied to pulleys
    // Attention: could be problematic for larger values.
    float mult = 10000.0f;

    b2PulleyJointDef pulleyDef;

    b2Vec2 anchor1 = body1->GetPosition();
    b2Vec2 anchor2 = body2->GetPosition();
    b2Vec2 groundAnchor1(dirClone1.x * mult, dirClone1.y * mult);
    b2Vec2 groundAnchor2(dirClone2.x * mult, dirClone2.y * mult);
    pulleyDef.Initialize(body1, body2, groundAnchor1, groundAnchor2, anchor1, anchor2, 1.0f);
    body1->GetWorld()->CreateJoint(&pulleyDef);

    rotateVec(&dirClone1, b2_pi / 2.0f);
    rotateVec(&dirClone2, b2_pi / 2.0f);

    anchor1 = body1->GetPosition();
    anchor2 = body2->GetPosition();
    groundAnchor1 = b2Vec2(dirClone1.x * mult, dirClone1.y * mult);
    groundAnchor2 = b2Vec2(dirClone2.x * mult, dirClone2.y * mult);
    pulleyDef.Initialize(body1, body2, groundAnchor1, groundAnchor2, anchor1, anchor2, 1.0f);
    body1->GetWorld()->CreateJoint(&pulleyDef);
}

void Portal::creation() {
    for (int i = 0; i < addBodies.size(); i++) {
        Shape* shape = addShapes.at(i);
        shape->applyData();

        connectedPortal->collidingFixtures.insert(shape->body->GetFixtureList());
        connectBodies(shape->body, addBodies.at(i));

        correspondingBodies[addBodies.at(i)] = shape->body;
        connectedPortal->correspondingBodies[shape->body] = addBodies.at(i);
    }
}

void Portal::destruction() {
    for (b2Body* destroyBody : destroyQueue) {
        destroyBody->GetWorld()->DestroyBody(destroyBody);
    }
    addShapes.clear();
    addBodies.clear();
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
    this->color = b2Color(1.0f, ((float)rand()) / RAND_MAX, ((float)rand()) / RAND_MAX, 1.0f);
    connectedPortal->color = (b2Color(1.0f-this->color.r, 1.0f - this->color.g, 1.0f - this->color.b, 1.0f));
}