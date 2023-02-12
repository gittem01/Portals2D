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

    portalConnection* pc;
    bool isRotated;

    int32 m_indexA;
	int32 m_indexB;
	float m_invMassA;
	float m_invMassB;
	float m_mass;
    float m_impulse;

    PositionJoint(const PositionJointDef* def) : BaseCustomJoint(def)
    {
        this->dirA = def->dirA;
        this->dirB = def->dirB;

        this->pc = def->pc;
        this->isRotated = def->isRotated;

        updateParameters();

        this->m_impulse = 0.0f;
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
        m_invMassA = m_bodyA->m_invMass;
        m_invMassB = m_bodyB->m_invMass;

        b2Vec2 cA = data.positions[m_indexA].c;
        b2Vec2 vA = data.velocities[m_indexA].v;

        b2Vec2 cB = data.positions[m_indexB].c;
        b2Vec2 vB = data.velocities[m_indexB].v;

        m_mass = m_invMassA + m_invMassB;

        if (m_mass > 0.0f)
        {
            m_mass = 1.0f / m_mass;
        }

        if (data.step.warmStarting)
        {
            m_impulse *= data.step.dtRatio;

            b2Vec2 PA = -m_impulse * dirA;
            b2Vec2 PB = -m_impulse * dirB;

            vA += m_invMassA * PA;
            vB += m_invMassB * PB;
        }
        else
        {
            m_impulse = 0.0f;
        }

        data.velocities[m_indexA].v = vA;
        data.velocities[m_indexB].v = vB;
    }

    void SolveVelocityConstraints(const b2SolverData& data) override
    {
        b2Vec2 vA = data.velocities[m_indexA].v;
        b2Vec2 vB = data.velocities[m_indexB].v;

        b2Vec2 velVecA;
        b2Vec2 centerVecA = m_bodyA->GetPosition() - pc->portal1->pos;
        velVecA = PortalWorld::rotateVec(centerVecA, b2_pi * 0.5f);
        velVecA *= pc->portal1->getAngVel();

        b2Vec2 velVecB;
        b2Vec2 centerVecB = m_bodyB->GetPosition() - pc->portal2->pos;
        velVecB = PortalWorld::rotateVec(centerVecB, b2_pi * 0.5f);
        velVecB *= pc->portal2->getAngVel();

        float Cdot = -b2Dot(dirA, vA - pc->portal1->getLinVel() - velVecA)
                     -b2Dot(dirB, vB - pc->portal2->getLinVel() - velVecB);

        vA += 0.5f * Cdot * dirA;
        vB += 0.5f * Cdot * dirB;

        m_impulse += -m_mass * Cdot;

        data.velocities[m_indexA].v = vA;
        data.velocities[m_indexB].v = vB;
    }

    bool SolvePositionConstraints(const b2SolverData& data) override
    {
        b2Vec2 cA = data.positions[m_indexA].c;
        b2Vec2 cB = data.positions[m_indexB].c;

        float mA = m_invMassA;
        float mB = m_invMassB;

        float mass = mA + mB;

        if (mass > 0.0f)
        {
            mass = 1.0f / mass;
        }

        float distA = PortalWorld::getRealDist(groundLineA[0], groundLineA[1], cA);
        float distB = PortalWorld::getRealDist(groundLineB[0], groundLineB[1], cB);

        float C = distA + distB;
        float linearError = b2Abs(C);

        float impulse = -mass * C;

        b2Vec2 PA = -impulse * dirA;
        b2Vec2 PB = -impulse * dirB;

        cA -= m_invMassA * PA;
        cB -= m_invMassB * PB;

        data.positions[m_indexA].c = cA;
        data.positions[m_indexB].c = cB;

        return linearError < b2_linearSlop;
    }
};