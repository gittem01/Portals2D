#include "Portal.h"
#include "glm/common.hpp"

#define CIRCLE_POINTS 50
#define DRAW_RELEASES 0

/*
    There is a buggy rendering going on when a release part of the
    fixture collided with another portal without exiting first portal
    This will be resolved when collision handling added
*/

std::vector<PortalBody*> PortalBody::portalBodies;

PortalBody::PortalBody(b2Body* body, b2World* world, b2Vec3 bodyColor){
    this->world = world;
    this->bodyMaps[body] = new std::vector<void*>();
    
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
    if (bData && bData->type == PORTAL){
        Portal* p = (Portal*)bData->data;
        int out = p->collisionBegin(contact, fix2, fix1);
        handleOut(fix1, (void*)p, out);
    }
}

void PortalBody::collisionEnd(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2){
    bodyData* bData = (bodyData*)fix2->GetBody()->GetUserData().pointer;
    if (bData && bData->type == PORTAL){
        Portal* p = (Portal*)bData->data;
        int out = p->collisionEnd(contact, fix2, fix1);
        handleOut(fix1, (void*)p, out);
    }
}

void PortalBody::preCollision(b2Contact* contact, b2Fixture* fix1, b2Fixture* fix2){
    bodyData* bData = (bodyData*)fix2->GetBody()->GetUserData().pointer;

    if (bData && bData->type == PORTAL){
        Portal* p = (Portal*)bData->data;
        int out = p->preCollision(contact, fix2, fix1);
        handleOut(fix1, (void*)p, out);
    }
}

void PortalBody::outHelper(b2Fixture* fix, void* portal, int status, int side){
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

void PortalBody::handleOut(b2Fixture* fix, void* portal, int out){
    switch (out)
    {
    case 0:
        outHelper(fix, portal, 1, 0);
        break;
    
    case 1:
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
        break;
    }
}

void PortalBody::drawBodies(){
    for (std::map<b2Body*, std::vector<void*>*>::iterator iter = bodyMaps.begin(); iter != bodyMaps.end(); iter++){
        for (b2Fixture* fix = iter->first->GetFixtureList(); fix != nullptr; fix = fix->GetNext()){
            if (fix->GetType() == b2Shape::Type::e_polygon){
                drawPolygonFix(fix);
            }
            else if (fix->GetType() == b2Shape::Type::e_circle){
                drawCircleFix(fix);
            }
        }
    }
}

void PortalBody::adjustVertices(std::vector<b2Vec2>& vertices, std::vector<b2Vec2>& retVertices1,
                                std::vector<b2Vec2>& retVertices2, void* p, int side){
    Portal* portal = (Portal*)p;

    int pSide;
    int lastSide = portal->getPointSide(vertices[0]);
    for (int i = 0; i < vertices.size() + 1; i++){
        int vertexIndex = i % vertices.size();
        pSide = portal->getPointSide(vertices[vertexIndex]);
        if (pSide == side && lastSide == 1 - side){
            b2Vec2 intersectionPoint = getLineIntersection(portal->points[0], portal->points[1], vertices[i - 1], vertices[vertexIndex]);
            retVertices1.push_back(intersectionPoint);
            retVertices2.push_back(intersectionPoint);
        }
        else if (pSide == 1 - side && lastSide == side){
            b2Vec2 intersectionPoint = getLineIntersection(portal->points[0], portal->points[1], vertices[i - 1], vertices[vertexIndex]);
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

    if (renderStatus == 0){
#if DRAW_RELEASES == 1
            b2Vec3 oldColor = bodyColor;
            bodyColor = b2Vec3(0.1f, 0.1f, 0.1f);
            drawVertices(body, vertices);
            bodyColor = oldColor;
#endif
    }
    else if (renderStatus == 1){
        std::vector<b2Vec2> drawVecs;
        std::vector<std::vector<b2Vec2>> allReleases;
        
        if (iter != (*fixIter).second->end()){
            for(;;){
                std::vector<b2Vec2> releaseVecs;
                adjustVertices(vertices, drawVecs, releaseVecs, (Portal*)(*iter)->portal, s);
                
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
            b2Vec3 oldColor = bodyColor;
            bodyColor = b2Vec3(1.0f, 1.0f, 1.0f);
            drawVertices(body, drawVecs);
            bodyColor = oldColor;
        }
#if DRAW_RELEASES == 1
        for (auto& vecs : allReleases){
            b2Vec3 oldColor = bodyColor;
            bodyColor = b2Vec3(0.1f, 0.1f, 0.1f);
            drawVertices(body, vecs);
            bodyColor = oldColor;
        }
#endif
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
        transparency /= 2;
    }
    
    glColor4f(bodyColor.x, bodyColor.y, bodyColor.z, 0.5f * transparency);
	glBegin(GL_POLYGON);
	for (int i = 0; i < vertices.size(); i++) {
		glVertex2d(vertices.at(i).x, vertices.at(i).y);
	}
	glEnd();

	glLineWidth(1.0f);
	glColor4f(bodyColor.x, bodyColor.y, bodyColor.z, transparency);
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