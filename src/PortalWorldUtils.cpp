#include "PortalBody.h"
#include "Portal.h"
#include <RotationJoint.h>
#include <PositionJoint.h>
#include <GameBase.h>
#include "GameObjects/GndLaserReceiver.h"
#include "GameObjects/LaserEmitter.h"

b2Vec2 PortalWorld::getRelativePortalPosition_L(b2Vec2 point, portalConnection* pConnect)
{
    Portal* portal1 = pConnect->portal1;
    Portal* portal2 = pConnect->portal2;
    int side1 = pConnect->side1;
    int side2 = pConnect->side2;
    bool reversed = pConnect->isReversed;

    b2Vec2 dir1 = side1 ? -portal1->dir : portal1->dir;
    b2Vec2 dir2 = side2 ? -portal2->dir : portal2->dir;

    float angleRot = -calcAngle2(dir1) + calcAngle2(-dir2);
    b2Vec2 pos = rotateVec(point, angleRot + b2_pi);
    if (reversed)
    {
        pos = mirror(dir2, pos);
    }

    return pos;
}

b2Vec2 PortalWorld::getRelativePortalPositionReverse_L(b2Vec2 point, portalConnection* pConnect)
{
    Portal* portal1 = pConnect->portal2;
    Portal* portal2 = pConnect->portal1;
    int side1 = pConnect->side2;
    int side2 = pConnect->side1;
    bool reversed = pConnect->isReversed;

    b2Vec2 dir1 = side1 ? -portal1->dir : portal1->dir;
    b2Vec2 dir2 = side2 ? -portal2->dir : portal2->dir;

    float angleRot = -calcAngle2(dir1) + calcAngle2(-dir2);
    b2Vec2 pos = rotateVec(point, angleRot + b2_pi);
    if (reversed)
    {
        pos = mirror(dir2, pos);
    }

    return pos;
}

b2Vec2 PortalWorld::getRelativePortalPosition_W(b2Vec2 point, portalConnection* pConnect)
{
    Portal* portal1 = pConnect->portal1;
    Portal* portal2 = pConnect->portal2;
    int side1 = pConnect->side1;
    int side2 = pConnect->side2;
    bool reversed = pConnect->isReversed;

    b2Vec2 dir1 = side1 ? -portal1->dir : portal1->dir;
    b2Vec2 dir2 = side2 ? -portal2->dir : portal2->dir;

    b2Vec2 posDiff = portal1->pos - point;
    float angleRot = -calcAngle2(dir1) + calcAngle2(-dir2);
    b2Vec2 pos = rotateVec(posDiff, angleRot + b2_pi);
    if (reversed)
    {
        pos = mirror(dir2, pos);
    }

    pos += portal2->pos;

    return pos;
}

b2Vec2 PortalWorld::getRelativePortalPositionReverse_W(b2Vec2 point, portalConnection* pConnect)
{
    Portal* portal1 = pConnect->portal2;
    Portal* portal2 = pConnect->portal1;
    int side1 = pConnect->side2;
    int side2 = pConnect->side1;
    bool reversed = pConnect->isReversed;

    b2Vec2 dir1 = side1 ? -portal1->dir : portal1->dir;
    b2Vec2 dir2 = side2 ? -portal2->dir : portal2->dir;

    b2Vec2 posDiff = portal1->pos - point;
    float angleRot = -calcAngle2(dir1) + calcAngle2(-dir2);
    b2Vec2 pos = rotateVec(posDiff, angleRot + b2_pi);
    if (reversed)
    {
        pos = mirror(dir2, pos);
    }

    pos += portal2->pos;

    return pos;
}

glm::vec4 PortalWorld::tangenter(point p1, point p2, float mult)
{
    if (p1.r == 0.0 && p2.r == 0.0)
    {
        return glm::vec4(p2.pos.x, p2.pos.y, p1.pos.x, p1.pos.y);
    }

    float mult0;
    if (p1.pos.x < p2.pos.x) mult0 = -1.0;
    else mult0 = 1.0f;

    if (p1.pos.x < p2.pos.x) mult *= -1.0;

    point bp;
    point sp;

    if (p1.r > p2.r)
    {
        bp = p1;
        sp = p2;
    }
    else
    {
        bp = p2;
        sp = p1;
    }

    bp = p1;
    sp = p2;

    float gamma = -atan((bp.pos.y - sp.pos.y) / (bp.pos.x - sp.pos.x));
    float beta = asin((bp.r - sp.r) / sqrt(pow(bp.pos.x - sp.pos.x, 2.0) + pow(bp.pos.y - sp.pos.y, 2.0))) * mult0 * mult;
    float alpha = gamma - beta;

    glm::vec2 v1 = glm::vec2(sp.pos.x + sp.r * sin(alpha) * mult, sp.pos.y + sp.r * cos(alpha) * mult);
    glm::vec2 v2 = glm::vec2(bp.pos.x + bp.r * sin(alpha) * mult, bp.pos.y + bp.r * cos(alpha) * mult);

    return glm::vec4(v1.x, v1.y, v2.x, v2.y);
}

void PortalWorld::calculateOuterEdges(std::vector<b2Shape*> shapes, std::vector<point>& outerVertices, std::vector<glm::vec4>& outerEdges, std::vector<float>& angles)
{
    if (shapes.size() == 1 && shapes[0]->GetType() == b2Shape::e_circle)
    {
        b2CircleShape* cShape = (b2CircleShape*)shapes[0];
        angles.push_back(0);
        outerEdges.push_back(glm::vec4(cShape->m_p.x, cShape->m_p.y, cShape->m_p.x, cShape->m_p.y));
        outerVertices.push_back({ { cShape->m_p.x, cShape->m_p.y }, cShape->m_radius });

        return;
    }

    std::vector<point> allPoints;
    for (b2Shape* shape : shapes)
    {
        if (shape->GetType() == b2Shape::e_polygon)
        {
            b2PolygonShape* pShape = (b2PolygonShape*)shape;
            for (int i = 0; i < pShape->m_count; i++)
            {
                allPoints.push_back({ { pShape->m_vertices[i].x, pShape->m_vertices[i].y }, 0.0});
            }
        }
        else
        {
            b2CircleShape* cShape = (b2CircleShape*)shape;
            allPoints.push_back({ { cShape->m_p.x, cShape->m_p.y }, cShape->m_radius });
        }
    }

    float maxX = -1e38;
    point startPoint;
    for (point p : allPoints)
    {
        if (p.pos.x + p.r > maxX)
        {
            maxX = p.pos.x + p.r;
            startPoint = p;
        }
        else if (abs(p.pos.x + p.r - maxX) < 1e-6 && p.pos.y - p.r < startPoint.pos.y - startPoint.r)
        {
            startPoint = p;
        }
    }

    outerVertices.push_back(startPoint);
    b2Vec2 lastVec = b2Vec2(0.0, 1.0);

    for (;;)
    {
        float minAngle = b2_pi * 2.345763;
        point addVertex{};
        b2Vec2 addVec;
        float addAngle = 0.0f;
        glm::vec4 addRes {};
        for (point p : allPoints)
        {
            if (startPoint.pos.x == p.pos.x && startPoint.pos.y == p.pos.y)
            {
                continue;
            }

            glm::vec4 res = tangenter(startPoint, p, 1.0);
            b2Vec2 vec = b2Vec2(res.x - res.z, res.y - res.w);
            float len = vec.Normalize();

            float ang1 = PortalWorld::calcAngle2(vec);
            float ang2 = PortalWorld::calcAngle2(lastVec);
            float angle = ang1 - ang2;
            if (angle < 0.0) angle += b2_pi * 2.0;
            if (angle < b2_pi + 1e-6 && len > 0.0)
            {
                if (angle < minAngle)
                {
                    minAngle = angle;
                    addVertex = p;
                    addVec = vec;
                    addRes = res;
                    addAngle = ang1;
                }
                else if (abs(angle - minAngle) < 1e-6 && glm::length(p.pos - startPoint.pos) + p.r > glm::length(addVertex.pos - startPoint.pos) + addVertex.r)
                {
                    minAngle = angle;
                    addVertex = p;
                    addVec = vec;
                    addRes = res;
                    addAngle = ang1;
                }
            }
        }

        if (outerEdges.size() > 0 && outerEdges[0].x == addRes.x && outerEdges[0].y == addRes.y)
        {
            break;
        }

        startPoint = addVertex;
        lastVec = addVec;

        outerVertices.push_back(addVertex);
        outerEdges.push_back(addRes);

        addAngle -= b2_pi * 0.5f;
        if (angles.size() > 0 && addAngle < angles[angles.size() - 1])
        {
            addAngle += 2.0f * b2_pi;
        }
        angles.push_back(fmax(addAngle, 0.0));
    }

    outerVertices.erase(outerVertices.end() - 1);

    assert(angles[0] >= 0.0);
}

float PortalWorld::getDistanceFromVec(b2Vec2 p0, b2Vec2 p1, PortalBody* portalBody)
{
    b2Vec2 dir = p0 - p1; dir = b2Vec2(-dir.y, dir.x);

    float dirAng = PortalWorld::calcAngle2(PortalWorld::rotateVec(dir, -portalBody->body->GetAngle()));
    std::vector<float> angles;
    std::vector<point> outerVertices;
    if (portalBody->isMirrored)
    {
        angles = portalBody->worldIndex->mirroredAngles;
        outerVertices = portalBody->worldIndex->mirroredOuterVertices;
    }
    else
    {
        angles = portalBody->worldIndex->angles;
        outerVertices = portalBody->worldIndex->outerVertices;
    }

    int s = 0;
    int e = angles.size() - 1;
    for (; e - s > 1; )
    {
        int p = (e - s) / 2 + s;
        if (angles[p] < dirAng)
        {
            s = p;
        }
        else
        {
            e = p;
        }
    }

    if (angles[0] > dirAng || angles[angles.size() - 1] < dirAng) e = 0;

    b2Vec2 worldPos = portalBody->body->GetWorldPoint(b2Vec2(outerVertices[e].pos.x, outerVertices[e].pos.y));

    return PortalWorld::getRealDist(p0, p1, worldPos) + outerVertices[e].r;
}

float PortalWorld::getDistanceFromPortal(Portal* portal, PortalBody* portalBody)
{
    float dirAng = PortalWorld::calcAngle2(PortalWorld::rotateVec(-portal->dir, -portalBody->body->GetAngle()));
    std::vector<float> angles;
    std::vector<point> outerVertices;
    if (portalBody->isMirrored)
    {
        angles = portalBody->worldIndex->mirroredAngles;
        outerVertices = portalBody->worldIndex->mirroredOuterVertices;
    }
    else
    {
        angles = portalBody->worldIndex->angles;
        outerVertices = portalBody->worldIndex->outerVertices;
    }

    int s = 0;
    int e = angles.size() - 1;
    for (; e - s > 1; )
    {
        int p = (e - s) / 2 + s;
        if (angles[p] < dirAng)
        {
            s = p;
        }
        else
        {
            e = p;
        }
    }

    if (angles[0] > dirAng || angles[angles.size() - 1] < dirAng) e = 0;

    b2Vec2 worldPos = portalBody->body->GetWorldPoint(b2Vec2(outerVertices[e].pos.x, outerVertices[e].pos.y));

    return PortalWorld::getRealDist(portal->points[0], portal->points[1], worldPos) + outerVertices[e].r;
}

PortalRay::PortalRay(PortalWorld* pWorld)
{
    this->pWorld = pWorld;
}

void PortalRay::prepareRaySend()
{
    rayResult.clear();
    groundReceivers.clear();

    closestPortal = nullptr;
    closestFixture = nullptr;
    closestPortalFixture = nullptr;
    filterBody = nullptr;
    filterMask = 0;
}

#include <ThinDrawer.h>
void PortalRay::sendRay(b2Vec2 rayStart, b2Vec2 dirVec, float rayLength, void* filterBody, uint32_t filterMask, int rayIndex, Portal* rayOutPortal)
{
    prepareRaySend();
    this->filterBody = filterBody;
    this->filterMask = filterMask;

    sendRay_i(rayStart, dirVec, rayLength, rayIndex, rayOutPortal);

    for (int i = 0; i < rayResult.size(); i++)
    {
        rayData res = rayResult[i];
        ThinDrawer::object->registerDebugLine(glm::vec4(res.startPos.x, res.startPos.y, res.endPos.x, res.endPos.y), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), 0.02f, 0.5f);
    }
}

void PortalRay::sendRay_i(b2Vec2 rayStart, b2Vec2 dirVec, float rayLength, int rayIndex, Portal* rayOutPortal)
{
    if (rayIndex >= maxRayCount || rayLength < 0.0001019f)
    {
        return;
    }

    PortalWorld::normalize(&dirVec);
    b2Vec2 rayEnd = rayStart + rayLength * dirVec;
    b2Vec2 diff = rayEnd - rayStart;

    reset();

    pWorld->RayCast(this, rayStart, rayEnd);

    endHandle();

    float minFrac = minFraction;

    diff = minFrac * diff;
    b2Vec2 collPoint = rayStart + diff;

    if (closestFixture != NULL)
    {
        if (closestFixture == closestPortalFixture)
        {
            fixtureData* fData = (fixtureData*)closestFixture->GetUserData().pointer;
            Portal* p = (Portal*)fData->data;
            int raySide = p->getPointSide(rayStart);

            for (portalConnection* conn : p->connections[raySide])
            {
                b2Vec2 ray2Pos = pWorld->getRelativePortalPosition_W(collPoint, conn);

                b2Vec2 dir2 = -pWorld->getRelativePortalPosition_L(dirVec, conn);

                ray2Pos -= 0.01f * dir2;

                sendRay_i(ray2Pos, dir2, rayLength - minFrac * rayLength, rayIndex + 1, conn->portal2);
            }
        }

        rayResult.push_back({ closestFixture, rayStart, collPoint, dirVec, rayOutPortal });
    }
    else
    {   
        rayResult.push_back({ closestFixture, rayStart, collPoint, dirVec, rayOutPortal });
    }

    leftLength = rayLength;
}

float PortalRay::ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float fraction)
{
    if (fixture->IsSensor() || (fixture->GetType() == b2Shape::e_polygon && fixture->GetDensity() < 0.3f)) return minFraction;

    bool isPortal = false;
    fixtureData* fData = (fixtureData*)fixture->GetUserData().pointer;
    if (fData)
    {
        if (fData->type == F_PORTAL)
        {
            Portal* p = (Portal*)fData->data;
            int side = p->getPointSide(point + normal);
            if (p->connections[side].size() != 0)
            {
                isPortal = true;
            }
            else if (p->connections[side ^ 1].size() != 0)
            {
                return minFraction;
            }
        }
        else if (fData->type == F_PORTAL_BODY)
        {
            bodyData* bData = (bodyData*)fixture->GetBody()->GetUserData().pointer;
            PortalBody* pb = (PortalBody*)bData->data;

            if (pb->worldIndex == filterBody) return minFraction;

            PBFixtureData* pbfData = (PBFixtureData*)fData->data;
            std::vector<portalCollision> collidingPortals = pbfData->pCollisions;
            for (portalCollision pc : collidingPortals)
            {
                if (pc.status < 1 || PortalWorld::isLeft(pc.portal->points[1], pc.portal->points[0], point, 0.000001f) == (pc.side ^ 1))
                {
                    return minFraction;
                }
            }
        }
        else if ((uint32_t)fData->type & filterMask)
        {
            if (fData->type & F_GND_RECEIVER && (filterMask == LSR_FILTER_MASK))
            {
                GndLaserReceiver* receiver = (GndLaserReceiver*)fData->data;
                groundReceivers.push_back({ receiver->arrIndex, fraction });
            }

            return minFraction;
        }
    }

    if (isPortal && fraction < portalFraction)
    {
        portalFraction = fraction;
        closestPortalFixture = fixture;

        fixtureData* fData = (fixtureData*)fixture->GetUserData().pointer;
        closestPortal = (Portal*)fData->data;
    }

    if (fraction <= minFraction)
    {
        minFraction = fraction;
        closestFixture = fixture;
        endNormal = normal;
    }

    return minFraction;
}

void PortalRay::reset()
{
    this->minFraction = 1.0f;
    this->portalFraction = 1.0f;

    this->closestFixture = NULL;
    this->closestPortalFixture = NULL;
}

void PortalRay::endHandle()
{
    if (this->minFraction > this->portalFraction)
    {
        this->closestFixture = this->closestPortalFixture;
        this->minFraction = this->portalFraction;
    }
}

b2Vec2 PortalWorld::rotateVec(b2Vec2 vec, float angle)
{
    float x = cos(angle) * vec.x - sin(angle) * vec.y;
    float y = sin(angle) * vec.x + cos(angle) * vec.y;

    return { x, y };
}

// it checks for left (really)
bool PortalWorld::isLeft(b2Vec2 a, b2Vec2 b, b2Vec2 c, const float t)
{
    return ((b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x)) >= t;
}

float PortalWorld::vecAngle(b2Vec2 v1, b2Vec2 v2)
{
    return abs(calcAngle2(v1) - calcAngle2(v2));
}

// left side gives positive values
float PortalWorld::getDist(b2Vec2 a, b2Vec2 b, b2Vec2 c)
{
    return ((b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x));
}

float PortalWorld::getRealDist(b2Vec2 a, b2Vec2 b, b2Vec2 c)
{
    float v1 = (b.x - a.x) * (a.y - c.y);
    float v2 = (a.x - c.x) * (b.y - a.y);
    float top = v1 - v2;

    v1 = pow(b.x - a.x, 2);
    v2 = pow(b.y - a.y, 2);
    float bot = sqrt(v1 + v2);

    return top / bot;
}

// project a on b
b2Vec2 PortalWorld::project(b2Vec2 a, b2Vec2 b)
{
    if (b.x == 0.0f && b.y == 0.0f) return b2Vec2(0.0f, 0.0f);
    return (b2Dot(a, b) / b2Dot(b, b)) * b;
}

b2Vec2 PortalWorld::closestPointToLine(b2Vec2 linePoint1, b2Vec2 linePoint2, b2Vec2 point)
{
    b2Vec2 lineVec = linePoint2 - linePoint1;

    b2Vec2 perpLineVec = b2Vec2(lineVec.y, -lineVec.x);

    b2Vec2 pointToLine1 = linePoint1 - point;

    b2Vec2 toResult = project(pointToLine1, perpLineVec);

    b2Vec2 resPos = point + toResult;

    return resPos;
}

float PortalWorld::calcAngle2(b2Vec2 vec) {
    float angle = atan2(vec.y, vec.x);
    if (angle < 0) angle += b2_pi * 2.0f;

    return angle;
}

void PortalWorld::normalize(b2Vec2* vec) {
    float length = vec->Length();
    if (length == 0) { return; }

    vec->x /= length;
    vec->y /= length;
}

b2Vec2 PortalWorld::mirror(b2Vec2 mirror, b2Vec2 vec)
{
    float angle1 = calcAngle2(mirror);
    float angle2 = calcAngle2(vec);
    return rotateVec(vec, 2 * (angle1 - angle2));
}

// normalize angle between -pi and pi
float PortalWorld::normalizeAngle(float angle)
{
    angle = fmod(angle, 2 * b2_pi);
    if (angle < -b2_pi)
        angle += 2 * b2_pi;
    else if (angle > b2_pi)
        angle -= 2 * b2_pi;

    return angle;
}

b2Vec2 PortalWorld::getLineIntersection(b2Vec2 p1, b2Vec2 p2, b2Vec2 p3, b2Vec2 p4)
{
    float d = (p1.x - p2.x) * (p3.y - p4.y) - (p1.y - p2.y) * (p3.x - p4.x);
    float up1 = (p1.x * p2.y - p1.y * p2.x) * (p3.x - p4.x) - (p1.x - p2.x) * (p3.x * p4.y - p3.y * p4.x);
    float up2 = (p1.x * p2.y - p1.y * p2.x) * (p3.y - p4.y) - (p1.y - p2.y) * (p3.x * p4.y - p3.y * p4.x);

    if (abs(d) < 0.0001f) d = 0.0001f;

    return b2Vec2(up1 / d, up2 / d);
}

bool PortalWorld::intersectionCheck(b2Vec2 p11, b2Vec2 p12, b2Vec2 p21, b2Vec2 p22)
{
    b2Vec2 intersectPoint = getLineIntersection(p11, p12, p21, p22);

    /*float len1 = (p12 - p11).LengthSquared();
    float len2 = (p22 - p21).LengthSquared();

    float len3 = (intersectPoint - p11).LengthSquared();
    float len4 = (intersectPoint - p21).LengthSquared();

    return len3 < len1 && len3 > 0.0f && len4 < len2 - 0.004f && len4 > 0.004f;*/

    float len1 = b2Dot(p12 - p11, intersectPoint - p11);
    float len2 = b2Dot(p22 - p21, intersectPoint - p21);

    float len3 = (p12 - p11).LengthSquared();
    float len4 = (p22 - p21).LengthSquared();

    return len3 > len1 && len1 > 0.0f && len4 > len2 && len2 > 0.0f;
}