#pragma once

#include <BaseCustomJoint.h>


struct PositionJointDef : public b2JointDef
{
    b2Vec2 groundLineA[2];
    b2Vec2 groundLineB[2];

    b2Vec2 dirA;
    b2Vec2 dirB;

    portalConnection* pc;
    bool isRotated;

    PositionJointDef()
	{
        type = e_unknownJoint;
	}

	void Initialize(b2Body* bA, b2Body* bB, portalConnection* c, bool isRotated=false)
    {
        this->bodyA = bA;
        this->bodyB = bB;
        this->pc = c;
        this->isRotated = isRotated;
    }
};

class PositionJoint : BaseCustomJoint
{

friend class PortalWorld;

private:

    b2Vec2 groundLineA[2];
    b2Vec2 groundLineB[2];

    b2Vec2 dirA;
    b2Vec2 dirB;

    b2Vec2 lastSpeedA;
    b2Vec2 speedChangeA;

    b2Vec2 lastSpeedB;
    b2Vec2 speedChangeB;

    b2Vec2 rotLastSpeedA;
    b2Vec2 rotSpeedChangeA;

    b2Vec2 rotLastSpeedB;
    b2Vec2 rotSpeedChangeB;

    portalConnection* pc;
    bool isRotated;

    int32 m_indexA;
	int32 m_indexB;

    PositionJoint(const PositionJointDef* def) : BaseCustomJoint(def)
    {
        this->jointType = POSITION;

        this->dirA = def->dirA;
        this->dirB = def->dirB;

        this->pc = def->pc;
        this->isRotated = def->isRotated;

        updateParameters();

        /*this->rotLastSpeedA = b2Vec2(0.0f, 0.0f);
        this->rotLastSpeedB = b2Vec2(0.0f, 0.0f);

        b2Vec2 reflectedSpeed;

        reflectedSpeed = pc->portal2->pos - PortalWorld::getRelativePortalPosition(pc->portal1->pos + pc->portal1->getLinVel(), pc);
        reflectedSpeed = PortalWorld::project(reflectedSpeed, dirB);
        this->lastSpeedA = reflectedSpeed;

        reflectedSpeed = pc->portal1->pos - PortalWorld::getRelativePortalPositionReverse(pc->portal2->pos + pc->portal2->getLinVel(), pc);
        reflectedSpeed = PortalWorld::project(reflectedSpeed, dirA);
        this->lastSpeedB = reflectedSpeed;*/
    }

    void updateParameters()
    {
        groundLineA[0] = pc->portal1->pos;
        groundLineB[0] = pc->portal2->pos;

        if (isRotated)
        {
            dirA = pc->side1 == 0 ? -pc->portal1->rotDir : pc->portal1->rotDir;
            dirB = pc->side2 == 0 ? -pc->portal2->rotDir : pc->portal2->rotDir;

            if (pc->side1 == 0)
            {
                groundLineA[1] = pc->portal1->pos + pc->portal1->dir;
            }
            else
            {
                groundLineA[1] = pc->portal1->pos - pc->portal1->dir;
            }
            if (pc->side2 == 0)
            {
                groundLineB[1] = pc->portal2->pos + pc->portal2->dir;
            }
            else
            {
                groundLineB[1] = pc->portal2->pos - pc->portal2->dir;
            }

            if (pc->isReversed)
            {
                groundLineA[0] = groundLineA[1];
                groundLineA[1] = pc->portal1->pos;
                dirA = -dirA;
            }
        }
        else
        {
            groundLineA[1] = pc->portal1->points[pc->side1];
            groundLineB[1] = pc->portal2->points[pc->side2];

            dirA = pc->side1 == 0 ? pc->portal1->dir : -pc->portal1->dir;
            dirB = pc->side2 == 0 ? pc->portal2->dir : -pc->portal2->dir;
        }
    }

    void InitVelocityConstraints(const b2SolverData& data) override
    {
        updateParameters();

        m_indexA = m_bodyA->m_islandIndex;
        m_indexB = m_bodyB->m_islandIndex;

        b2Vec2 cA = data.positions[m_indexA].c;
        b2Vec2 vA = data.velocities[m_indexA].v;

        b2Vec2 cB = data.positions[m_indexB].c;
        b2Vec2 vB = data.velocities[m_indexB].v;

        data.velocities[m_indexA].v = vA;
        data.velocities[m_indexB].v = vB;
    }

    void SolveVelocityConstraints(const b2SolverData& data) override
    {
        b2Vec2 vA = data.velocities[m_indexA].v;
        b2Vec2 vB = data.velocities[m_indexB].v;

        b2Vec2 centerVecA = m_bodyA->GetPosition() - pc->portal1->pos;
        b2Vec2 velVecA = PortalWorld::rotateVec(centerVecA, b2_pi * 0.5f);
        velVecA *= pc->portal1->getAngVel();
        velVecA = PortalWorld::getRelativePortalPosition_L(velVecA, pc);
        velVecA = PortalWorld::project(velVecA, dirB);

        b2Vec2 centerVecB = m_bodyB->GetPosition() - pc->portal2->pos;
        b2Vec2 velVecB = PortalWorld::rotateVec(centerVecB, b2_pi * 0.5f);
        velVecB *= pc->portal2->getAngVel();
        velVecB = PortalWorld::getRelativePortalPositionReverse_L(velVecB, pc);
        velVecB = PortalWorld::project(velVecB, dirA);

        b2Vec2 relVel1 = PortalWorld::getRelativePortalPosition_L(pc->portal1->getLinVel(), pc);
        b2Vec2 relVel2 = PortalWorld::getRelativePortalPositionReverse_L(pc->portal2->getLinVel(), pc);
        
        float Cdot =    -b2Dot(dirA, vA - relVel2 - velVecB)
                        -b2Dot(dirB, vB - relVel1 - velVecA);
        
        vA += 0.5f * Cdot * dirA;
        vB += 0.5f * Cdot * dirB;

        data.velocities[m_indexA].v = vA;
        data.velocities[m_indexB].v = vB;
    }

    bool SolvePositionConstraints(const b2SolverData& data) override
    {
        b2Vec2 cA = data.positions[m_indexA].c;
        b2Vec2 cB = data.positions[m_indexB].c;

        float distA = PortalWorld::getRealDist(groundLineA[0], groundLineA[1], cA);
        float distB = PortalWorld::getRealDist(groundLineB[0], groundLineB[1], cB);

        float C = distA + distB;
        float linearError = b2Abs(C);

        b2Vec2 PA = C * dirA;
        b2Vec2 PB = C * dirB;

        cA -= 0.5f * PA;
        cB -= 0.5f * PB;

        data.positions[m_indexA].c = cA;
        data.positions[m_indexB].c = cB;

        return linearError < b2_linearSlop;
    }
};