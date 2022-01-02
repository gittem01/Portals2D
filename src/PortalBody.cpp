#include "Portal.h"
#include "glm/common.hpp"

#define CIRCLE_POINTS 50

bool PortalBody::drawReleases = true;
b2Color PortalBody::releaseColor = b2Color(1.0f, 1.0f, 1.0f, 0.2f);
std::vector<PortalBody*> PortalBody::portalBodies;
std::set<PortalBody*> PortalBody::destroyBodies;

PortalBody::PortalBody(b2Body* body, b2World* world, b2Color bodyColor){
    this->world = world;
    this->_body = body;
    this->bodyMaps = new std::vector<bodyCollisionStatus*>();

    bodyData* bd = (bodyData*)malloc(sizeof(bodyData));
    bd->data = this;
    bd->type = PORTAL_BODY;
    body->GetUserData().pointer = (uintptr_t)bd;

    this->bodyColor = bodyColor;
    PortalBody::portalBodies.push_back(this);

    for (b2Fixture* fix = body->GetFixtureList(); fix; fix = fix->GetNext()){
        fixtureCollisions[fix] = new std::set<portalCollision*>();
    }
}

void PortalBody::collisionBegin(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2){
    bodyData* bData = (bodyData*)fix2->GetBody()->GetUserData().pointer;
    
    bool shouldCollide = this->shouldCollide(contact, fix1, fix2, bData);
    if (!shouldCollide){
        return;
    }

    if (bData && bData->type == PORTAL){
        Portal* p = (Portal*)bData->data;
        int out = p->collisionBegin(contact, fix2, fix1);
        handleOut(fix1, p, out);
    }
}

void PortalBody::collisionEnd(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2){
    bodyData* bData = (bodyData*)fix2->GetBody()->GetUserData().pointer;
    
    bool shouldCollide = this->shouldCollide(contact, fix1, fix2, bData);
    if (!shouldCollide){
        return;
    }

    if (bData && bData->type == PORTAL){
        Portal* p = (Portal*)bData->data;
        int out = p->collisionEnd(contact, fix2, fix1);
        handleOut(fix1, p, out);
    }
}

void PortalBody::preCollision(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2){
    bodyData* bData = (bodyData*)fix2->GetBody()->GetUserData().pointer;

    bool shouldCollide = this->shouldCollide(contact, fix1, fix2, bData);
    if (!shouldCollide){
        contact->SetEnabled(false);
        return;
    }

    if (bData && bData->type == PORTAL){
        Portal* portal = (Portal*)bData->data;
        int out = portal->preCollision(contact, fix2, fix1);
        handleOut(fix1, portal, out);
    }
}

bool PortalBody::shouldCollide(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2, bodyData* bData){
    std::set<portalCollision*>* collidingPortals = fixtureCollisions[fix1];
    if (bData && bData->type == PORTAL){
        Portal* portal = (Portal*)bData->data;
        for (portalCollision* coll : *collidingPortals){
            if (portal == coll->portal) return true;
        }
    }
    
    for (portalCollision* coll : *collidingPortals){
        if (coll->portal->releaseFixtures[coll->side].find(fix2) != coll->portal->releaseFixtures[coll->side].end() ||
            coll->portal->collidingFixtures[coll->side].find(fix2) != coll->portal->collidingFixtures[coll->side].end())
        {
            return true;
        }

        if (coll->status == 0) return false;
        bool shouldCollide = coll->portal->shouldCollide(contact, fix1, fix2, coll);
        if (!shouldCollide) return false;
    }
    
    return true;
}

void PortalBody::outHelper(b2Fixture* fix, Portal* portal, int status, int side){
    for (auto& col : *fixtureCollisions[fix]){
        if (col->portal == portal){
            fixtureCollisions[fix]->erase(col);
            break;
        }
    }
    portalCollision* portCol = (portalCollision*)malloc(sizeof(portalCollision));
    portCol->portal = portal;
    portCol->status = status;
    portCol->side = side;
    fixtureCollisions[fix]->insert(portCol);
}

void PortalBody::globalPostHandle(b2World* world){
    for (PortalBody* b : destroyBodies){
        for (b2Fixture* fix = b->_body->GetFixtureList(); fix; fix = fix->GetNext()){
            for (Portal* p : Portal::portals){
                for (int i = 0; i < 2; i++){
                    p->collidingFixtures[i].erase(fix);
                    p->releaseFixtures[i].erase(fix);
                }
            }
        }

        for (int i = 0; i < b->bodyMaps->size(); i++){
            bodyCollisionStatus* body1Status = b->bodyMaps->at(i);
            for (auto iter = body1Status->body->bodyMaps->begin(); iter != body1Status->body->bodyMaps->end(); std::advance(iter, 1)){
                if ((*iter)->body == b){
                    delete(*iter);
                    body1Status->body->bodyMaps->erase(iter);
                    break;
                }
            }
        }

        world->DestroyBody(b->_body);
        for (int i = 0 ; i < PortalBody::portalBodies.size(); i++){
            if (PortalBody::portalBodies.at(i) == b){
                PortalBody::portalBodies.erase(PortalBody::portalBodies.begin() + i);
                delete(b);
                break;
            }
        }
    }
}

void PortalBody::postHandle(){
    for (bodyStruct* s : createBodies){
        createCloneBody(s->body, s->collPortal, s->side);
    }

    createBodies.clear();
    destroyBodies.clear();
}

bool PortalBody::shouldCreate(b2Body* body, Portal* portal, int side){
    for (bodyCollisionStatus* s : *bodyMaps){
        if (s->connection->portal1 == portal && s->connection->side1 == side)
            return false;
    }

    return true;
}

void PortalBody::handleOut(b2Fixture* fix, Portal* portal, int out){
    switch (out)
    {
    case 0:
        if (shouldCreate(fix->GetBody(), portal, 0)){
            for (portalConnection* c : portal->connections[0]){
                bodyCollisionStatus* bcs = new bodyCollisionStatus;
                bcs->body = nullptr;
                bcs->connection = c;
                bodyMaps->push_back(bcs);
            }

            bodyStruct* s = (bodyStruct*)malloc(sizeof(bodyStruct));
            *s = {fix->GetBody(), portal, 0};
            createBodies.push_back(s);
        }
        outHelper(fix, portal, 1, 0);
        break;
    case 1:
        if (shouldCreate(fix->GetBody(), portal, 1)){
            for (portalConnection* c : portal->connections[1]){
                bodyCollisionStatus* bcs = new bodyCollisionStatus;
                bcs->body = nullptr;
                bcs->connection = c;
                bodyMaps->push_back(bcs);
            }

            bodyStruct* s = (bodyStruct*)malloc(sizeof(bodyStruct));
            *s = {fix->GetBody(), portal, 1};
            createBodies.push_back(s);
        }
        outHelper(fix, portal, 1, 1);
        break;
    
    case 2:
        outHelper(fix, portal, 0, 0);
        break;
    case 3:
        outHelper(fix, portal, 0, 1);
        break;

    case 4:
        std::set<portalCollision*>* coll = fixtureCollisions[fix];
        for (auto& a : *coll){
            if (a->portal == portal && a->status == 1){
                coll->erase(a);
                free(a);
                break;
            }
        }
        destroyCheck(fix->GetBody(), portal);
        break;
    }
}

void PortalBody::destroyCheck(b2Body* body, Portal* portal){
    for (b2Fixture* fix = body->GetFixtureList(); fix; fix  = fix->GetNext()){
        for (portalCollision* coll : *fixtureCollisions[fix]){
            if (coll->portal == portal){
                return;
            }
        }
    }

    for (bodyCollisionStatus* s : *bodyMaps){
        if (s->connection->portal1 == portal){
            destroyBodies.insert(s->body);
        }
    }
}

void rotateVec(b2Vec2* vec, float angle){
    float x = cos(angle) * vec->x - sin(angle) * vec->y;
    float y = sin(angle) * vec->x + cos(angle) * vec->y;
    vec->x = x;
    vec->y = y;
}

b2Vec2 rotateVec(b2Vec2 vec, float angle){
    float x = cos(angle) * vec.x - sin(angle) * vec.y;
    float y = sin(angle) * vec.x + cos(angle) * vec.y;

    return b2Vec2(x, y);
}

void PortalBody::createCloneBody(b2Body* body1, Portal* collPortal, int side){
    b2Vec2 dir1 = side ? -collPortal->dir : collPortal->dir;
    b2Vec2 posDiff = collPortal->pos - body1->GetPosition();
    b2Vec2 speed = body1->GetLinearVelocity();

    for (portalConnection* c : collPortal->connections[side]){
        Portal* portal2 = c->portal2;
        
        b2Vec2 dir2 = c->side2 ? -portal2->dir : portal2->dir;
        float angleRot = -Portal::calcAngle2(dir1) + Portal::calcAngle2(-dir2);

        b2Vec2 v = b2Vec2();

        b2BodyDef def;
        def.type = b2_dynamicBody;
        def.position = portal2->pos + rotateVec(posDiff, angleRot + b2_pi);
        def.linearDamping = body1->GetLinearDamping();
        def.angularDamping = body1->GetAngularDamping();

        b2Body* body2 = world->CreateBody(&def);

        bodyData* bd = (bodyData*)malloc(sizeof(bodyData));
        bd->data = this;
        bd->type = PORTAL_BODY;
        body2->GetUserData().pointer = (uintptr_t)bd;

        bodyCollisionStatus* bcs = new bodyCollisionStatus;

        PortalBody* npb = new PortalBody(body2, world, bodyColor);
        npb->bodyMaps->push_back(bcs);

        bcs->body = this;
        for (portalConnection* conn : portal2->connections[c->side2]){
            if (conn->portal2 == collPortal && conn->side2 == c->side1){
                bcs->connection = conn;
                break;
            }
        }

        for (bodyCollisionStatus* s : *bodyMaps){
            if (s->connection == c){
                s->body = npb;
                break;
            }
        }

        body2->SetLinearVelocity(rotateVec(speed, angleRot));
        body2->SetAngularVelocity(body1->GetAngularVelocity());
        body2->SetTransform(body2->GetPosition(), body1->GetTransform().q.GetAngle());

        for (b2Fixture* fix = body1->GetFixtureList(); fix; fix = fix->GetNext()){
            b2Fixture* f;
            if (fix->GetType() == b2Shape::Type::e_polygon){
                b2PolygonShape* polyShape = (b2PolygonShape*)fix->GetShape();
                b2FixtureDef fDef;
                fDef.density = fix->GetDensity();
                b2PolygonShape newShape;
                fDef.shape = &newShape;

                newShape.m_count = polyShape->m_count;
                b2Vec2* vertices = (b2Vec2*)malloc(sizeof(b2Vec2) * polyShape->m_count);

                for (int i = 0; i < polyShape->m_count; i++){
                    vertices[i] = rotateVec(polyShape->m_vertices[i], angleRot);
                }
                newShape.Set(vertices, polyShape->m_count);
                f = body2->CreateFixture(&fDef);
            }
            else{
                b2CircleShape* circleShape = (b2CircleShape*)fix->GetShape();
                b2FixtureDef fDef;
                fDef.density = fix->GetDensity();
                b2CircleShape newShape;
                fDef.shape = &newShape;

                newShape.m_radius = circleShape->m_radius;
                newShape.m_p = rotateVec(circleShape->m_p, angleRot);
                
                f = body2->CreateFixture(&fDef);
            }

            npb->fixtureCollisions[f] = new std::set<portalCollision*>();

            portalCollision* col = (portalCollision*)malloc(sizeof(portalCollision));;
            col->portal = portal2;
            col->side = c->side2;

            if (collPortal->collidingFixtures[side].find(fix) != collPortal->collidingFixtures[side].end()){
                portal2->collidingFixtures[c->side2].insert(f);
                col->status = 1;
            }
            else if (collPortal->releaseFixtures[side].find(fix) == collPortal->releaseFixtures[side].end()){
                portal2->releaseFixtures[c->side2].insert(f);
                col->status = 0;
            }

            npb->fixtureCollisions[f]->insert(col);   
        }

        connectBodies(body1, body2, c, side);
    }
}

void PortalBody::connectBodies(b2Body* body1, b2Body* body2, portalConnection* connection, int side) {
    b2PrismaticJointDef prismDef;
    prismDef.Initialize(body1, body2, b2Vec2(0.0f, 0.0f), b2Vec2(0.0f, 0.0f));
    prismDef.collideConnected = true;
    prismDef.maxMotorForce = 0.0f;

    body1->GetWorld()->CreateJoint(&prismDef);

    b2Vec2 dirClone1 = connection->side1 == 0 ? connection->portal1->dir : -connection->portal1->dir;
    b2Vec2 dirClone2 = connection->side2 == 0 ? connection->portal2->dir : -connection->portal2->dir;

    float mult = 100000.0f;

    b2PulleyJointDef pulleyDef;

    b2Vec2 anchor1 = body1->GetPosition();
    b2Vec2 anchor2 = body2->GetPosition();
    b2Vec2 groundAnchor1(dirClone1.x * mult, dirClone1.y * mult);
    b2Vec2 groundAnchor2(dirClone2.x * mult, dirClone2.y * mult);
    pulleyDef.Initialize(body1, body2, groundAnchor1, groundAnchor2, anchor1, anchor2, 1.0f);
    world->CreateJoint(&pulleyDef);

    rotateVec(&dirClone1, b2_pi / 2.0f);
    rotateVec(&dirClone2, b2_pi / 2.0f);

    anchor1 = body1->GetPosition();
    anchor2 = body2->GetPosition();
    groundAnchor1 = b2Vec2(dirClone1.x * mult, dirClone1.y * mult);
    groundAnchor2 = b2Vec2(dirClone2.x * mult, dirClone2.y * mult);
    pulleyDef.Initialize(body1, body2, groundAnchor1, groundAnchor2, anchor1, anchor2, 1.0f);
    world->CreateJoint(&pulleyDef);
}

void PortalBody::drawBodies(){
    for (b2Fixture* fix = _body->GetFixtureList(); fix != nullptr; fix = fix->GetNext()){
        if (fix->GetType() == b2Shape::Type::e_polygon){
            drawPolygonFix(fix);
        }
        else if (fix->GetType() == b2Shape::Type::e_circle){
            drawCircleFix(fix);
        }
    }
}

void PortalBody::adjustVertices(std::vector<b2Vec2>& vertices, std::vector<b2Vec2>& retVertices1,
                                std::vector<b2Vec2>& retVertices2, Portal* p, int side)
{
    int pSide;
    int lastSide = p->getPointSide(vertices[0]);
    for (int i = 0; i < vertices.size() + 1; i++){
        int vertexIndex = i % vertices.size();
        pSide = p->getPointSide(vertices[vertexIndex]);
        if (pSide == side && lastSide == 1 - side){
            b2Vec2 intersectionPoint = getLineIntersection(p->points[0], p->points[1], vertices[i - 1], vertices[vertexIndex]);
            retVertices1.push_back(intersectionPoint);
            retVertices2.push_back(intersectionPoint);
        }
        else if (pSide == 1 - side && lastSide == side){
            b2Vec2 intersectionPoint = getLineIntersection(p->points[0], p->points[1], vertices[i - 1], vertices[vertexIndex]);
            retVertices1.push_back(intersectionPoint);
            retVertices2.push_back(intersectionPoint);
        }

        if (pSide == side && i != vertices.size()){
            retVertices1.push_back(vertices[vertexIndex]);
        }
        else if (i != vertices.size()){
            retVertices2.push_back(vertices[vertexIndex]);
        }

        lastSide = pSide;
    }
}

void PortalBody::portalRender(b2Fixture* fix, std::vector<b2Vec2>& vertices){
    b2Body* body = fix->GetBody();

    int side;
    int renderStatus = 2;

    std::map<b2Fixture*, std::set<portalCollision*>*>::iterator fixIter = fixtureCollisions.find(fix);
    std::set<portalCollision*>::iterator iter = (*fixIter).second->begin();
    
    void* portal = NULL;
    int s = -1;
    if (iter != (*fixIter).second->end()){
        portal = (*iter)->portal;
        s = (*iter)->side;
        renderStatus = (*iter)->status;
    }
    for (portalCollision* coll : *(*fixIter).second){
        if (coll->status == 0){
            renderStatus = 0;
            break;
        }
        else{
            renderStatus = 1;
        }
    }

    if (renderStatus == 1){
        std::vector<b2Vec2> drawVecs;
        std::vector<std::vector<b2Vec2>> allReleases;
        
        if (iter != (*fixIter).second->end()){
            for(;;){
                std::vector<b2Vec2> releaseVecs;
                adjustVertices(vertices, drawVecs, releaseVecs, (*iter)->portal, s);
                
                vertices.clear();
                for (b2Vec2 v : drawVecs){
                    vertices.push_back(v);
                }

                allReleases.push_back(releaseVecs);
                
                std::advance(iter, 1);
                if (iter == (*fixIter).second->end()) break;

                drawVecs.clear();
            }
        }

        if (drawVecs.size() > 0){
            drawVertices(body, drawVecs);
        }
        if (drawReleases){
            for (auto& vecs : allReleases){
                b2Color oldColor = bodyColor;
                bodyColor = releaseColor;
                drawVertices(body, vecs);
                bodyColor = oldColor;
            }
        }
    }
    else if (drawReleases && renderStatus == 0){
            b2Color oldColor = bodyColor;
            bodyColor = releaseColor;
            drawVertices(body, vertices);
            bodyColor = oldColor;
    }

    else if (renderStatus == 2) drawVertices(body, vertices);
}

void PortalBody::drawPolygonFix(b2Fixture* fix){
    b2Body* body = fix->GetBody();
    b2PolygonShape* shape = (b2PolygonShape*)fix->GetShape();

    int vertexCount = shape->m_count;
    std::vector<b2Vec2> vertices;

	for (int i = 0; i < vertexCount; i++) {
        b2Vec2 p = b2Vec2((shape->m_vertices + i)->x, (shape->m_vertices + i)->y);
        p = body->GetWorldPoint(p);
        vertices.push_back(p);
	}

    portalRender(fix, vertices);
}

void PortalBody::drawCircleFix(b2Fixture* fix){
    b2Body* body = fix->GetBody();
    b2CircleShape* shape = (b2CircleShape*)fix->GetShape();
    float r = shape->m_radius;

    std::vector<b2Vec2> vertices;

    float angle = 0;
	for (int i = 0; i < CIRCLE_POINTS; i++) {
        float angle0 = ((float)i / CIRCLE_POINTS) * b2_pi * 2;
        b2Vec2 p = b2Vec2(sin(angle0) * r, cos(angle0) * r) + shape->m_p;
        p = body->GetWorldPoint(p);
        vertices.push_back(p);
	}

    portalRender(fix, vertices);
}

void PortalBody::drawVertices(b2Body* body, std::vector<b2Vec2>& vertices){
    float transparency = 1.0f;
    if (!body->IsAwake()){
        transparency /= 1.5f;
    }
    
    glColor4f(bodyColor.r, bodyColor.g, bodyColor.b, bodyColor.a * transparency);
	glBegin(GL_POLYGON);
	for (int i = 0; i < vertices.size(); i++) {
		glVertex2d(vertices.at(i).x, vertices.at(i).y);
	}
	glEnd();

	glLineWidth(1.0f);
	glColor4f(bodyColor.r, bodyColor.g, bodyColor.b, bodyColor.a * 2.0f);
	glBegin(GL_LINES);
	for (int i = 0; i < vertices.size(); i++) {
		glVertex2d(vertices.at(i).x, vertices.at(i).y);
		glVertex2d(vertices.at((i + 1) % vertices.size()).x, vertices.at((i + 1) % vertices.size()).y);
	}
	glEnd();
}

b2Vec2 PortalBody::getLineIntersection(b2Vec2 p1, b2Vec2 p2, b2Vec2 p3, b2Vec2 p4){
    float d = (p1.x - p2.x) * (p3.y - p4.y) - (p1.y - p2.y) * (p3.x - p4.x);
    float up1 = (p1.x * p2.y - p1.y * p2.x) * (p3.x - p4.x) - (p1.x - p2.x) * (p3.x * p4.y - p3.y * p4.x);
    float up2 = (p1.x * p2.y - p1.y * p2.x) * (p3.y - p4.y) - (p1.y - p2.y) * (p3.x * p4.y - p3.y * p4.x);

    return b2Vec2(up1 / d, up2 / d);
}