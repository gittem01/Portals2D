#pragma once

#include <Portal.h>
#include <stdio.h>


struct PositionJointDef : public b2JointDef
{
    b2Vec2 groundLineA[2];
    b2Vec2 groundLineB[2];

    b2Vec2 dirA;
    b2Vec2 dirB;

    // may change for scaling portals later
    float ratio = 1.0f;

    PositionJointDef()
	{
        type = e_unknownJoint;
	}

	void Initialize(b2Body* bA, b2Body* bB, portalConnection* c, bool isRotated=false){
        bodyA = bA;
        bodyB = bB;

        dirA = c->side1 == 0 ? c->portal1->dir : -c->portal1->dir;
        dirB = c->side2 == 0 ? c->portal2->dir : -c->portal2->dir;

        if (isRotated){
            if (c->side1 == 0){
                groundLineA[0] = bA->GetPosition();
                groundLineA[1] = bA->GetPosition() + c->portal1->dir;
                dirA = PortalWorld::rotateVec(dirA, -b2_pi * 0.5f);
            }
            else{
                groundLineA[0] = bA->GetPosition();
                groundLineA[1] = bA->GetPosition() - c->portal1->dir;
                dirA = PortalWorld::rotateVec(dirA, -b2_pi * 0.5f);
            }
            if (c->side2 == 0){
                groundLineB[0] = bB->GetPosition();
                groundLineB[1] = bB->GetPosition() + c->portal2->dir;
                dirB = PortalWorld::rotateVec(dirB, -b2_pi * 0.5f);
            }
            else{
                groundLineB[0] = bB->GetPosition();
                groundLineB[1] = bB->GetPosition() - c->portal2->dir;
                dirB = PortalWorld::rotateVec(dirB, -b2_pi * 0.5f);
            }
            if (c->isReversed){
                b2Vec2 tempA = groundLineA[0];
                groundLineA[0] = groundLineA[1];
                groundLineA[1] = tempA;
                dirA = PortalWorld::rotateVec(dirA, b2_pi);
            }
        }
        else{
            groundLineA[0] = c->portal1->points[c->side1 ^ 1];
            groundLineA[1] = c->portal1->points[c->side1];
            groundLineB[0] = c->portal2->points[c->side2 ^ 1];
            groundLineB[1] = c->portal2->points[c->side2];
        }
    }
};

class PositionJoint : b2Joint
{

friend class PortalWorld;

public:

    b2Vec2 GetAnchorA() const override { return b2Vec2(); };
	b2Vec2 GetAnchorB() const override { return b2Vec2(); };

	b2Vec2 GetReactionForce(float inv_dt) const override { return b2Vec2(); };
	float GetReactionTorque(float inv_dt) const override { return 0.0f; };

private:

    b2Vec2 groundLineA[2];
    b2Vec2 groundLineB[2];

    b2Vec2 dirA;
    b2Vec2 dirB;

    float ratio;

    int32 m_indexA;
	int32 m_indexB;
	float m_invMassA;
	float m_invMassB;
	float m_mass;
    float m_impulse;

    PositionJoint(const PositionJointDef* def) : b2Joint(def)
    {
        groundLineA[0] = def->groundLineA[0];
        groundLineA[1] = def->groundLineA[1];
        groundLineB[0] = def->groundLineB[0];
        groundLineB[1] = def->groundLineB[1];

        dirA = def->dirA;
        dirB = def->dirB;

        ratio = def->ratio;

        m_impulse = 0.0f;
    }

    void InitVelocityConstraints(const b2SolverData& data) override
    {
        m_indexA = m_bodyA->m_islandIndex;
        m_indexB = m_bodyB->m_islandIndex;
        m_invMassA = m_bodyA->m_invMass;
        m_invMassB = m_bodyB->m_invMass;

        b2Vec2 cA = data.positions[m_indexA].c;
        b2Vec2 vA = data.velocities[m_indexA].v;

        b2Vec2 cB = data.positions[m_indexB].c;
        b2Vec2 vB = data.velocities[m_indexB].v;

        //m_mass = m_invMassA + ratio * ratio * m_invMassB;
        m_mass = m_invMassA + m_invMassB;

        if (m_mass > 0.0f)
        {
            m_mass = 1.0f / m_mass;
        }

        if (data.step.warmStarting)
        {
            m_impulse *= data.step.dtRatio;

            b2Vec2 PA = -(m_impulse) * dirA;
            //b2Vec2 PB = (-ratio * m_impulse) * dirB;
            b2Vec2 PB = (-m_impulse) * dirB;

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

        //float Cdot = -b2Dot(dirA, vpA) - m_ratio * b2Dot(dirA, vpB);
        float Cdot = -b2Dot(dirA, vA) - b2Dot(dirB, vB);
        float impulse = -m_mass * Cdot;
        m_impulse += impulse;

        b2Vec2 PA = -impulse * dirA;
        //b2Vec2 PB = -m_ratio * impulse * m_uB;
        b2Vec2 PB = -impulse * dirB;
        vA += m_invMassA * PA;
        vB += m_invMassB * PB;

        data.velocities[m_indexA].v = vA;
        data.velocities[m_indexB].v = vB;
    }

    bool SolvePositionConstraints(const b2SolverData& data) override
    {
        b2Vec2 cA = data.positions[m_indexA].c;
        b2Vec2 cB = data.positions[m_indexB].c;

        float mA = m_invMassA;
        float mB = m_invMassB;

        //float mass = mA + m_ratio * m_ratio * mB;
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
        b2Vec2 PB = -ratio * impulse * dirB;

        cA -= m_invMassA * PA;
        cB -= m_invMassB * PB;

        data.positions[m_indexA].c = cA;
        data.positions[m_indexB].c = cB;

        return linearError < b2_linearSlop;
    }
};