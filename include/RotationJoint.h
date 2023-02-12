#pragma once

#include <BaseCustomJoint.h>

struct RotationJointDef : public b2JointDef
{
    portalConnection* pc;
    float referenceAngle;

    RotationJointDef()
	{
		type = e_unknownJoint;
		referenceAngle = 0.0f;
	}

	void Initialize(b2Body* bA, b2Body* bB, portalConnection* pc){
        this->bodyA = bA;
        this->bodyB = bB;
        this->pc = pc;
	    this->referenceAngle = bodyB->GetAngle() - bodyA->GetAngle();
    }
};

class RotationJoint : BaseCustomJoint
{

friend class PortalWorld;

private:

    PortalBody* pBodyA;
    PortalBody* pBodyB;
    portalConnection* pc;
    float portalAStartAngle;
    float portalBStartAngle;
    float m_referenceAngle;

    int32 m_indexA;
	int32 m_indexB;
	float m_invMassA;
	float m_invMassB;
	float m_invIA;
	float m_invIB;
	b2Mat22 m_K;
    b2Vec2 m_impulse;

    RotationJoint(const RotationJointDef* def, PortalBody* pb1, PortalBody* pb2) : BaseCustomJoint(def)
    {
        this->jointType = ROTATION;

        pc = def->pc;
        pBodyA = pb1;
        pBodyB = pb2;
        m_impulse.SetZero();

        portalAStartAngle = pc->portal1->body->GetAngle();
        portalBStartAngle = pc->portal2->body->GetAngle();

        if (pc->isReversed)
        {
            float angle1 = pb1->body->GetAngle();
            float angle2 = pb2->body->GetAngle();
            m_referenceAngle = angle2 + angle1;
        }
        else
        {
            m_referenceAngle = def->referenceAngle;
        }
    }

    void InitVelocityConstraints(const b2SolverData& data) override
    {
        m_indexA = m_bodyA->m_islandIndex;
        m_indexB = m_bodyB->m_islandIndex;
        m_invMassA = m_bodyA->m_invMass;
        m_invMassB = m_bodyB->m_invMass;
        m_invIA = m_bodyA->m_invI;
        m_invIB = m_bodyB->m_invI;

        float wA = data.velocities[m_indexA].w;
        float wB = data.velocities[m_indexB].w;

        float mA = m_invMassA, mB = m_invMassB;
        float iA = m_invIA, iB = m_invIB;

        {
            float k11 = mA + mB;
            float k12 = 0.0f;
            float k22 = iA + iB;
            // for artifical kinematic bodies
            // if (k22 == 0.0f)
            // {
            //     k22 = 1.0f;
            // }

            m_K.ex.Set(k11, k12);
            m_K.ey.Set(k12, k22);
        }

        if (data.step.warmStarting && 0)
        {
            m_impulse *= data.step.dtRatio;

            if (pc->isReversed)
            {
                wA += iA * m_impulse.y;
            }
            else
            {
                wA -= iA * m_impulse.y;
            }
            wB += iB * m_impulse.y;
        }
        else
        {
            m_impulse.SetZero();
        }

        data.velocities[m_indexA].w = wA;
        data.velocities[m_indexB].w = wB;
    }

    void SolveVelocityConstraints(const b2SolverData& data) override
    {
        float wA = data.velocities[m_indexA].w;
        float wB = data.velocities[m_indexB].w;

        float iA = m_invIA, iB = m_invIB;

        if (m_invIA == 0.0f && m_invIB == 0.0f)
        {
            iA = 1.0f;
            iB = 1.0f;
        }

        b2Vec2 Cdot;
        Cdot.x = 0.0f;
        if (pc->isReversed)
        {
            Cdot.y = wB + wA - pc->portal1->getAngVel() - pc->portal2->getAngVel();
        }
        else
        {
            Cdot.y = wB - wA + pc->portal1->getAngVel() - pc->portal2->getAngVel();
        }

        b2Vec2 df = m_K.Solve(-Cdot);
        m_impulse += df;

        if (pc->isReversed)
        {
            wA += iA * df.y;
        }
        else
        {
            wA -= iA * df.y;
        }
        wB += iB * df.y;

        data.velocities[m_indexA].w = wA;
        data.velocities[m_indexB].w = wB;
    }

    bool SolvePositionConstraints(const b2SolverData& data) override
    {
        float aA = data.positions[m_indexA].a;
        float aB = data.positions[m_indexB].a;

        float mA, mB;
        float iA, iB;

        // checking only one because this joint should only connect kinematic-kinematic or dynamic-dynamic bodies
        if (m_invIA == 0.0f && m_invIB == 0.0f)
        {
            mA = 1.0f;
            iA = 1.0f;
            mB = 1.0f;
            iB = 1.0f;
        }
        else
        {
            mA = m_invMassA, mB = m_invMassB;
            iA = m_invIA, iB = m_invIB;
        }

        float lastAngle;
        if (pc->isReversed)
        {
            lastAngle = PortalWorld::normalizeAngle(aA + aB - m_referenceAngle +
                (portalAStartAngle - pc->portal1->body->GetAngle()) + (portalBStartAngle - pc->portal2->body->GetAngle()));
        }
        else
        {
            lastAngle = PortalWorld::normalizeAngle(aB - aA - m_referenceAngle -
                (portalAStartAngle - pc->portal1->body->GetAngle() - (portalBStartAngle - pc->portal2->body->GetAngle())));
        }

        b2Vec2 C1;
        C1.x = 0.0f;
        C1.y = lastAngle;

        float angularError = b2Abs(C1.y);

        float k11 = mA + mB;
        float k12 = 0;
        float k22 = iA + iB;
        // if (k22 == 0.0f)
        // {
        //     k22 = 1.0f;
        // }

        b2Mat22 K;
        K.ex.Set(k11, k12);
        K.ey.Set(k12, k22);

        b2Vec2 impulse1 = K.Solve(-C1);

        float LA = impulse1.y;
        float LB = impulse1.y;

        if (pc->isReversed)
        {
            aA += iA * LA;
        }
        else
        {
            aA -= iA * LA;
        }
        aB += iB * LB;

        data.positions[m_indexA].a = aA;
        data.positions[m_indexB].a = aB;

        return angularError <= b2_angularSlop;
    }
};