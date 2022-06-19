#include "PortalBody.h"
#include "Portal.h"
#include "PortalWorld.h"
#include "glm/common.hpp"

#define CIRCLE_POINTS 50

PortalBody::PortalBody(b2Body* bBody, PortalWorld* pWorld, b2Color bodyColor){
    this->pWorld = pWorld;
    this->body = bBody;
    this->bodyMaps = new std::vector<bodyCollisionStatus*>();

    this->body->SetGravityScale(0.0f);
    this->offsetAngle = 0;

    bodyData* bd = (bodyData*)malloc(sizeof(bodyData));
    bd->data = this;
    bd->type = PORTAL_BODY;
    bd->extraData = nullptr;
    body->GetUserData().pointer = (uintptr_t)bd;

    this->bodyColor = bodyColor;

    for (b2Fixture* fix = body->GetFixtureList(); fix; fix = fix->GetNext()){
        fixtureCollisions[fix] = new std::set<portalCollision*>();
        allParts[fix] = new std::vector<std::vector<b2Vec2>*>();
    }
}

PortalBody::~PortalBody(){
    for (b2Fixture* fix = body->GetFixtureList(); fix; fix = fix->GetNext()){
        for (Portal* p : pWorld->portals){
            for (int i = 0; i < 2; i++){
                p->collidingFixtures[i].erase(fix);
                p->releaseFixtures[i].erase(fix);
            }
        }
    }

    for (int i = 0; i < worldIndex->size(); i++){
        if (worldIndex->at(i) == this){
            worldIndex->erase(worldIndex->begin() + i);
            break;
        }
    }

    for (int i = 0; i < bodyMaps->size(); i++){
        bodyCollisionStatus* bodyCollStatus = bodyMaps->at(i);
        for (auto iter = bodyCollStatus->body->bodyMaps->begin(); iter != bodyCollStatus->body->bodyMaps->end(); iter++){
            if ((*iter)->body == this){
                delete *iter;
                bodyCollStatus->body->bodyMaps->erase(iter--);
            }
        }
        delete bodyCollStatus;
    }
    delete bodyMaps;

    for (b2Fixture* fix = body->GetFixtureList(); fix; fix = fix->GetNext()){
        for (std::vector<b2Vec2>* vec : *allParts[fix]){
            delete vec;
        }
        delete allParts[fix];
        
        for (portalCollision* coll : *fixtureCollisions[fix]){
            free(coll);
        }
        delete fixtureCollisions[fix];
    }

    uintptr_t bData = body->GetUserData().pointer;
    body->GetUserData().pointer = 0;
    free((void*)bData);

    pWorld->DestroyBody(body);
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

    bool sc = shouldCollide(contact, fix1, fix2, bData);
    if (!sc){
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

    if (fix2->GetBody()->GetType() == b2_staticBody && !fix2->IsSensor() && prepareMaps.find(fix1) != prepareMaps.end()){
        auto mapsFix = prepareMaps[fix1];
        for (auto iter = mapsFix.begin(); iter != mapsFix.end(); iter++){
            (*iter)->prepareCollisionCheck(contact, fix1, fix2);
        }
    }

    return true;
}

void PortalBody::outHelper(b2Fixture* fix, Portal* portal, int status, int side){
    for (auto& col : *fixtureCollisions[fix]){
        if (col->portal == portal){
            fixtureCollisions[fix]->erase(col);
            free(col);
            break;
        }
    }
    portalCollision* portCol = (portalCollision*)malloc(sizeof(portalCollision));
    portCol->portal = portal;
    portCol->status = status;
    portCol->side = side;
    fixtureCollisions[fix]->insert(portCol);
}

std::vector<PortalBody*> PortalBody::postHandle(){
    for (b2Fixture* fix = body->GetFixtureList(); fix; fix = fix->GetNext()){
        calculateParts(fix);
    }

    std::vector<PortalBody*> retBodies;

    for (bodyStruct* s : createBodies){
        std::vector<PortalBody*> bodies = pWorld->createCloneBody(s);
        for (int i = 0; i < bodies.size(); i++){
            retBodies.push_back(bodies.at(i));
        }
        free(s);
    }
    createBodies.clear();

    return retBodies;
}

bool PortalBody::shouldCreate(b2Body* bBody, Portal* portal, int side){
    bodyData* bd = (bodyData*)bBody->GetUserData().pointer;
    for (bodyCollisionStatus* s : *bodyMaps){
        const auto iter = pWorld->destroyBodies.find(s->body);
        if (iter != pWorld->destroyBodies.end() && s->connection->portal1 == portal){
            //pWorld->drawer->DrawPoint(s->body->body->GetPosition(), 50.0f, b2Color(1, 0, 0));
            pWorld->destroyBodies.erase(iter);
            break;
        }
    }
    
    for (bodyCollisionStatus* s : *bodyMaps){
        if (s->connection->portal1 == portal && s->connection->side1 == side)
            return false;
    }

    for (bodyStruct* bs : createBodies){
        if (bs->collPortal == portal && bs->side == side)
            return false;
    }

    return true;
}

void PortalBody::handleOut(b2Fixture* fix, Portal* portal, int out){
    switch (out)
    {
    case 0:
    case 1:
        if (shouldCreate(fix->GetBody(), portal, out)){
            bodyStruct* s = (bodyStruct*)malloc(sizeof(bodyStruct));
            *s = {this, portal, out};
            createBodies.push_back(s);
            //pWorld->drawer->DrawPoint(body->GetPosition() + b2Vec2(0.05f, 0.0f), 10.0f, b2Color(0.0f, 0.0f, 1.0f));
        }
        outHelper(fix, portal, 1, out);
        break;
    
    case 2:
        outHelper(fix, portal, 0, 0);
        break;
    case 3:
        outHelper(fix, portal, 0, 1);
        break;

    case 5:
        prepareMaps[fix].insert(portal);
        break;
    case 6:
        prepareMaps[fix].erase(portal);
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
        destroyCheck(fix, portal);
        break;
    }
}

void PortalBody::destroyCheck(b2Fixture* f, Portal* portal){
    for (b2Fixture* fix = f->GetBody()->GetFixtureList(); fix; fix = fix->GetNext()){
        if (fix == f) continue;
        for (portalCollision* coll : *fixtureCollisions[fix]){
            if (coll->portal == portal){
                return;
            }
        }
    }

    for (bodyCollisionStatus* s : *bodyMaps){
        if (s->connection->portal1 == portal){
            pWorld->destroyBodies.insert(s->body);
            //pWorld->drawer->DrawPoint(body->GetPosition() - b2Vec2(0.05f, 0.0f), 10.0f, b2Color(1.0f, 0.0f, 0.0f));
        }
    }
}

void rotateVec(b2Vec2* vec, float angle){
    float x = cos(angle) * vec->x - sin(angle) * vec->y;
    float y = sin(angle) * vec->x + cos(angle) * vec->y;
    vec->x = x;
    vec->y = y;
}

void PortalBody::drawBodies(){
    for (b2Fixture* fix = body->GetFixtureList(); fix != nullptr; fix = fix->GetNext()){
        if (fix->GetType() == b2Shape::Type::e_polygon){
            drawPolygonFix(fix);
        }
        else if (fix->GetType() == b2Shape::Type::e_circle){
            drawCircleFix(fix);
        }
    }
#if 0
    b2Transform t = body->GetTransform();
    t.q.Set(offsetAngle + body->GetAngle());
    pWorld->drawer->DrawTransform(t);
#endif
}

void PortalBody::adjustVertices(std::vector<b2Vec2>* vertices, std::vector<b2Vec2>* retVertices1,
                                std::vector<b2Vec2>* retVertices2, Portal* p, int side)
{
    int pSide;
    int lastSide = p->getPointSide(vertices->at(0));

    for (int i = 0; i < vertices->size() + 1; i++){
        int vertexIndex = i % vertices->size();
        int vertexBefore = i ? i - 1 : 0;
        pSide = p->getPointSide(vertices->at(vertexIndex));

        if (pSide == side && lastSide == 1 - side){
            b2Vec2 intersectionPoint = getLineIntersection(p->points[0], p->points[1], vertices->at(i - 1), vertices->at(vertexIndex));
            retVertices1->push_back(intersectionPoint);
            retVertices2->push_back(intersectionPoint);
        }
        else if (pSide == 1 - side && lastSide == side){
            b2Vec2 intersectionPoint = getLineIntersection(p->points[0], p->points[1], vertices->at(i - 1), vertices->at(vertexIndex));
            retVertices1->push_back(intersectionPoint);
            retVertices2->push_back(intersectionPoint);
        }

        if (pSide == side && i != vertices->size()){
            retVertices1->push_back(vertices->at(vertexIndex));
        }
        else if (i != vertices->size()){
            retVertices2->push_back(vertices->at(vertexIndex));
        }

        lastSide = pSide;
    }
}

float PortalBody::getArea(b2Fixture* fix, int status){
    auto apf = allParts[fix];

    float baseArea = 0.0f;
    float finalArea = 0.0f;
    float baseDensity = fix->GetDensity();
    if (fix->GetType() == b2Shape::Type::e_polygon){
        b2PolygonShape* pShape = (b2PolygonShape*)fix->GetShape();
        for (int i = 0; i < pShape->m_count; i++){
            baseArea += pShape->m_vertices[i].x * pShape->m_vertices[(i + 1) % pShape->m_count].y;
            baseArea -= pShape->m_vertices[(i + 1) % pShape->m_count].x * pShape->m_vertices[i].y;
        }
        baseArea *= 0.5f;
    }
    else{
        b2CircleShape* cShape = (b2CircleShape*)fix->GetShape();
        baseArea = pow(cShape->m_radius, 2) * b2_pi;
    }

    if (status == 2) return baseArea;

    std::vector<b2Vec2>* vecs = apf->at(0);

    for (int i = 0; i < vecs->size(); i++){
        b2Vec2 v0 = vecs->at(i);
        b2Vec2 v1 = vecs->at((i + 1) % vecs->size());

        finalArea += v0.x * v1.y;
        finalArea -= v1.x * v0.y;
    }
    finalArea /= 2.0f;
    
    return finalArea;
}

b2Vec2 findCentroid(std::vector<b2Vec2>* vecs){
    b2Vec2 off = vecs->at(0);
    float twicearea = 0;
    float x = 0;
    float y = 0;
    b2Vec2 p1, p2;
    float f;
    for (int i = 0, j = vecs->size() - 1; i < vecs->size(); j = i++) {
        p1 = vecs->at(i);
        p2 = vecs->at(j);
        f = (p1.x - off.x) * (p2.y - off.y) - (p2.x - off.x) * (p1.y - off.y);
        twicearea += f;
        x += (p1.x + p2.x - 2 * off.x) * f;
        y += (p1.y + p2.y - 2 * off.y) * f;
    }

    f = twicearea * 3;

    float rv1 = x / f + off.x;
    float rv2 = y / f + off.y;

    // Validity check
    if (!b2IsValid(rv1) || !b2IsValid(rv2)){
        return b2Vec2(0, 0);
    }

    return b2Vec2(rv1, rv2);
}

void PortalBody::applyGravity(b2Fixture* fix, int status){
    auto apf = allParts[fix];
    if (apf->size() == 0 || status == 0) return;

    std::vector<b2Vec2>* vecs = apf->at(0);
    if (vecs->size() == 0) return;

    float area = getArea(fix, status);

    if (area == 0) return;

    b2Vec2 center = findCentroid(vecs);

    b2Vec2 force = area * fix->GetDensity() * pWorld->GetGravity();

    fix->GetBody()->ApplyForce(((float)bodyMaps->size() + 1) * force, center, false);
}

void PortalBody::calculateParts(b2Fixture* fix){
    std::map<b2Fixture*, std::set<portalCollision*>*>::iterator fixIter = fixtureCollisions.find(fix);
    std::set<portalCollision*>::iterator setIter = (*fixIter).second->begin();

    std::vector<b2Vec2>* vertices = new std::vector<b2Vec2>;

    int status = 2;
    for (portalCollision* coll : *(*fixIter).second){
        if (coll->status == 0){
            status = 0;
            break;
        }
        else{
            status = 1;
        }
    }

    if (fix->GetType() == b2Shape::Type::e_polygon){
        b2PolygonShape* shape = (b2PolygonShape*)fix->GetShape();
        for (int i = 0; i < shape->m_count; i++) {
            b2Vec2 p = b2Vec2((shape->m_vertices + i)->x, (shape->m_vertices + i)->y);
            p = fix->GetBody()->GetWorldPoint(p);
            vertices->push_back(p);
        }
    }
    else{
        b2CircleShape* shape = (b2CircleShape*)fix->GetShape();
        float r = shape->m_radius;

        float angle = 0;
        for (int i = 0; i < CIRCLE_POINTS; i++) {
            float angle0 = -((float)i / CIRCLE_POINTS) * b2_pi * 2;
            b2Vec2 p = b2Vec2(sin(angle0) * r, cos(angle0) * r) + shape->m_p;
            p = fix->GetBody()->GetWorldPoint(p);
            vertices->push_back(p);
        }
    }

    std::vector<std::vector<b2Vec2>*>* apf = allParts[fix];

    for (std::vector<b2Vec2>* vec : *apf){
        delete vec;
    }

    apf->clear();

    if (status == 2){
        apf->push_back(vertices);
        applyGravity(fix, status);
        return;
    }

    std::vector<b2Vec2>* drawVecs = new std::vector<b2Vec2>;
    apf->push_back(drawVecs);
    
    if (setIter != (*fixIter).second->end()){
        for(;;){
            if (vertices->size() == 0) break;

            std::vector<b2Vec2>* releaseVecs = new std::vector<b2Vec2>;

            adjustVertices(vertices, drawVecs, releaseVecs, (*setIter)->portal, (*setIter)->side);

            vertices->clear();
            for (b2Vec2 v : *drawVecs){
                vertices->push_back(v);
            }
            if (releaseVecs->size() != 0)
                apf->push_back(releaseVecs);
            else
                free(releaseVecs);

            setIter++;
            if (setIter == (*fixIter).second->end()) break;

            drawVecs->clear();
        }
    }
    delete(vertices);

    applyGravity(fix, status);
}

void PortalBody::portalRender(b2Fixture* fix, std::vector<b2Vec2>& vertices){
    b2Body* bBody = fix->GetBody();

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
        if (allParts[fix]->at(0)->size() > 0){
            drawVertices(bBody, *allParts[fix]->at(0));
        }
        if (pWorld->drawReleases){
            for (int i = 1; i < allParts[fix]->size(); i++){
                auto& vecs = allParts[fix]->at(i);
                b2Color oldColor = bodyColor;
                bodyColor = pWorld->releaseColor;
                drawVertices(bBody, *vecs);
                bodyColor = oldColor;
            }
        }
    }
    else if (pWorld->drawReleases && renderStatus == 0){
            b2Color oldColor = bodyColor;
            bodyColor = pWorld->releaseColor;
            drawVertices(bBody, vertices);
            bodyColor = oldColor;
    }

    else if (renderStatus == 2) drawVertices(bBody, *allParts[fix]->at(0));
}

void PortalBody::drawPolygonFix(b2Fixture* fix){
    b2Body* bBody = fix->GetBody();
    b2PolygonShape* shape = (b2PolygonShape*)fix->GetShape();

    int vertexCount = shape->m_count;
    std::vector<b2Vec2> vertices;

	for (int i = 0; i < vertexCount; i++) {
        b2Vec2 p = b2Vec2((shape->m_vertices + i)->x, (shape->m_vertices + i)->y);
        p = bBody->GetWorldPoint(p);
        vertices.push_back(p);
	}

    portalRender(fix, vertices);
}

void PortalBody::drawCircleFix(b2Fixture* fix){
    b2Body* bBody = fix->GetBody();
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

void PortalBody::drawVertices(b2Body* bBody, std::vector<b2Vec2>& vertices){
    float transparency = 1.0f;
    if (!bBody->IsAwake()){
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
