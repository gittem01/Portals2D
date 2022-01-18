#include "PortalWorld.h"
#include "PortalBody.h"
#include "Portal.h"

PortalWorld::PortalWorld(b2World* world, DebugDrawer* drawer){
    this->world = world;
    this->drawer = drawer;

    this->drawReleases = false;
    this->releaseColor = b2Color(1.0f, 1.0f, 1.0f, 0.2f);
}

Portal* PortalWorld::createPortal(b2Vec2 pos, b2Vec2 dir, float size){
    Portal* portal = new Portal(pos, dir, size, this);
    portals.push_back(portal);

    return portal;
}

PortalBody* PortalWorld::createPortalBody(b2Body* body, b2Color bodyColor){
    PortalBody* pBody = new PortalBody(body, this, bodyColor);
    portalBodies.push_back(pBody);

    return pBody;
}

void PortalWorld::portalUpdate(){
    for (int i = 0; i < portalBodies.size(); i++){
        PortalBody* body = portalBodies.at(i);
        body->postHandle();
    }
    for (Portal* p : portals) {
        p->postHandle();
    }

    globalPostHandle();
}

void PortalWorld::drawUpdate(){
    for (PortalBody* body : portalBodies){
        body->drawBodies();
    }
    for (Portal* p : portals) {
        p->draw();
    }
}

void PortalWorld::globalPostHandle(){
    for (PortalBody* b : destroyBodies){
        delete b;

        for (int i = 0 ; i < portalBodies.size(); i++){
            if (portalBodies.at(i) == b){
                portalBodies.erase(portalBodies.begin() + i);
                break;
            }
        }
    }

    destroyBodies.clear();
}


bool PortalWorld::isLeft(b2Vec2& a, b2Vec2& b, b2Vec2& c, float t){
     return ((b.x - a.x)*(c.y - a.y) - (b.y - a.y)*(c.x - a.x)) >= t;
}

float PortalWorld::vecAngle(b2Vec2 v1, b2Vec2 v2){
    return abs(calcAngle2(v1) - calcAngle2(v2));
}

float PortalWorld::getDist(b2Vec2& a, b2Vec2& b, b2Vec2& c){
     return ((b.x - a.x)*(c.y - a.y) - (b.y - a.y)*(c.x - a.x));
}

float PortalWorld::calcAngle2(b2Vec2 vec) {
    float angle = atan2(vec.y, vec.x);
    if (angle < 0) angle += b2_pi * 2.0f;

    return angle;
}

void PortalWorld::normalize(b2Vec2* vec){
    float length = vec->Length();
    if (length == 0) { return; }
    
    vec->x /= length;
    vec->y /= length;
}