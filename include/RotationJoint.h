#pragma once

#include <PortalBody.h>
#include <stdio.h>

struct RotationJointDef : public b2JointDef
{
    float referenceAngle;

    RotationJointDef()
	{
		type = e_unknownJoint;
		referenceAngle = 0.0f;
	}

	void Initialize(b2Body* bA, b2Body* bB){
        bodyA = bA;
        bodyB = bB;
	    referenceAngle = bodyB->GetAngle() - bodyA->GetAngle();
    }
};

class RotationJoint : b2Joint
{

friend class PortalWorld;

public:

    b2Vec2 GetAnchorA() const override { return b2Vec2(); };
	b2Vec2 GetAnchorB() const override { return b2Vec2(); };

	b2Vec2 GetReactionForce(float inv_dt) const override { return b2Vec2(); };
	float GetReactionTorque(float inv_dt) const override { return inv_dt * m_impulse.y; };

private:

    PortalBody* pBodyA;
    PortalBody* pBodyB;
    float m_referenceAngle;
    bool isReversed;

    int32 m_indexA;
	int32 m_indexB;
	float m_invMassA;
	float m_invMassB;
	float m_invIA;
	float m_invIB;
	b2Mat22 m_K;
    b2Vec2 m_impulse;

    RotationJoint(const RotationJointDef* def, const bool rev, PortalBody* pb1, PortalBody* pb2, float pAngle) : b2Joint(def)
    {
        isReversed = rev;
        pBodyA = pb1;
        pBodyB = pb2;
        m_impulse.SetZero();

        if (isReversed)
        {
            float angle1 = pb1->body->GetAngle() + pb1->offsetAngle;
            float angle2 = pb2->body->GetAngle() + pb2->offsetAngle;
            m_referenceAngle = angle2 + angle1;
        }
        else{
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
            if (k22 == 0.0f)
            {
                k22 = 1.0f;
            }

            m_K.ex.Set(k11, k12);
            m_K.ey.Set(k12, k22);
        }

        if (data.step.warmStarting)
        {
            m_impulse *= data.step.dtRatio;

            if (isReversed){
                wA += iA * m_impulse.y;
            }
            else{
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

        b2Vec2 Cdot;
        Cdot.x = 0.0f;
        if (isReversed){
            Cdot.y = wB + wA;
        }
        else{
            Cdot.y = wB - wA;
        }

        b2Vec2 df = m_K.Solve(-Cdot);
        m_impulse += df;

        if (isReversed){
            wA += iA * df.y;
        }
        else{
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

        float mA = m_invMassA, mB = m_invMassB;
        float iA = m_invIA, iB = m_invIB;

        float lastAngle;

        if (isReversed){
            float angle1 = aA + pBodyA->offsetAngle;
            float angle2 = aB + pBodyB->offsetAngle;

            angle1 = fmod(angle1, b2_pi * 2);
            angle2 = fmod(angle2, b2_pi * 2);

            if (angle1 > b2_pi){
                angle1 -= b2_pi * 2;
            }
            else if (angle1 < -b2_pi){
                angle1 += b2_pi * 2;
            }

            if (angle2 > b2_pi){
                angle2 -= b2_pi * 2;
            }
            else if (angle2 < -b2_pi){
                angle2 += b2_pi * 2;
            }

            lastAngle = angle1 + angle2 - m_referenceAngle;
            lastAngle = fmod(lastAngle, b2_pi * 2);
            if (lastAngle > b2_pi){
                lastAngle -= b2_pi * 2;
            }
            else if (lastAngle < -b2_pi){
                lastAngle += b2_pi * 2;
            }
        }
        else{
            lastAngle = aB - aA - m_referenceAngle;
        }

        b2Vec2 C1;
        C1.x = 0.0f;
        C1.y = lastAngle;

        float angularError = b2Abs(C1.y);

        float k11 = mA + mB;
        float k12 = 0;
        float k22 = iA + iB;
        if (k22 == 0.0f)
        {
            k22 = 1.0f;
        }

        b2Mat22 K;
        K.ex.Set(k11, k12);
        K.ey.Set(k12, k22);

        b2Vec2 impulse1 = K.Solve(-C1);

        float LA = impulse1.y;
        float LB = impulse1.y;

        if (isReversed){
            aA += iA * LA;
        }
        else{
            aA -= iA * LA;
        }
        aB += iB * LB;

        data.positions[m_indexA].a = aA;
        data.positions[m_indexB].a = aB;

        return angularError <= b2_angularSlop;
    }
};