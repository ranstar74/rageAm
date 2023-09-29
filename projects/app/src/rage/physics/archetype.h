//
// File: archetype.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "bounds/boundbase.h"
#include "collisionflags.h"
#include "rage/atl/conststring.h"
#include "rage/math/constants.h"
#include "rage/paging/base.h"
#include "rage/paging/resource.h"

namespace rage
{
	enum phArchetypeClassType
	{
		ARCHETYPE_BASE,
		ARCHETYPE_PHYS,
		ARCHETYPE_DAMP,
	};

	class phArchetypeBase : public pgBase
	{
	protected:
		phArchetypeClassType m_Type;

	public:
		phArchetypeBase() = default;

		// ReSharper disable once CppPossiblyUninitializedMember
		phArchetypeBase(const datResource& rsc)
		{

		}

		phArchetypeClassType GetType() const { return m_Type; }
	};

	class phArchetype : public phArchetypeBase
	{
		atConstString	m_Filename;
		pgPtr<phBound>	m_Bound;
		CollisionFlags	m_TypeFlags;
		CollisionFlags	m_IncludeFlags;
		u16				m_PropertyFlags;
		s16				m_RefCount;

	public:
		phArchetype()
		{
			m_Type = ARCHETYPE_BASE;
			m_RefCount = 0;
			m_PropertyFlags = 0;
			m_TypeFlags = 0;
			m_IncludeFlags = 0;
		}

		// ReSharper disable once CppPossiblyUninitializedMember
		phArchetype(const datResource& rsc) : phArchetypeBase(rsc)
		{

		}

		void SetBound(phBound* bound) { m_Bound = bound; }
		phBound* GetBound() const { return m_Bound.Get(); }

		virtual void Erase();
		virtual void LoadData()
		{
			// This is unused in release game builds function that loads archetype from .phys files
		}
		virtual void CopyData(phArchetype* from);
		virtual void Copy() { /* TODO */ }
		virtual phArchetype* Clone() { /* TODO */ return nullptr; }
		virtual float GetMass() { return 1.0f; }
		virtual float GetInvMass() { return 1.0f; }
		virtual Vec3V GetAngInertia() { return S_ZERO; }
		virtual Vec3V GetInvAngInertia() { return S_ZERO; }
		virtual void SetMass(const ScalarV& mass) {}
		virtual void SetMass(float mass) {}
		virtual void SetAngInertia(const Vector3& angInertia) {}
		virtual void SetAngInertia(const Vec3V& angInertia) { SetAngInertia(angInertia); }
		virtual void SetGravityFactor(float gravityFactor) {}
		virtual float GetMaxSpeed() { return DEFAULT_MAX_SPEED; }
		virtual float GetMaxAngSpeed() { return DEFAULT_MAX_ANG_SPEED; }

		static phArchetype* Place(const datResource& rsc, phArchetype* that);

		IMPLEMENT_REF_COUNTER16(phArchetype);

	protected:
		static constexpr float DEFAULT_MAX_SPEED = 150.0f;
		static constexpr float DEFAULT_MAX_ANG_SPEED = CONST_2PI;
		static constexpr float DEFAULT_GRAVITY_FACTOR = 1.0f;
		static constexpr float DEFAULT_BUOYANCY_FACTOR = 1.0f;
	};

	class phArchetypePhys : public phArchetype
	{
		u64		m_Pad; // Required for mass 16 alignment, could also use alignas(16) though
		float	m_Mass;
		float	m_InvMass;
		float	m_GravityFactor;
		float	m_MaxSpeed;
		float	m_MaxAngSpeed;
		float	m_BuoyancyFactor;
		Vec3V	m_AngInertia;
		Vec3V	m_InvAngInertia;
	public:
		phArchetypePhys()
		{
			m_Type = ARCHETYPE_PHYS;

			m_Pad = 0;

			m_Mass = 1.0f;
			m_InvMass = 1.0f;

			m_AngInertia = S_ONE;
			m_InvAngInertia = S_ONE;

			m_MaxSpeed = DEFAULT_MAX_SPEED;
			m_MaxAngSpeed = DEFAULT_MAX_ANG_SPEED;

			m_GravityFactor = DEFAULT_GRAVITY_FACTOR;
			m_BuoyancyFactor = DEFAULT_BUOYANCY_FACTOR;
		}

		// ReSharper disable once CppPossiblyUninitializedMember
		phArchetypePhys(const datResource& rsc) : phArchetype(rsc)
		{

		}
	};

	class phArchetypeDamp : public phArchetypePhys
	{
		// C  = constant friction (doesn't depend on speed)
		// V  = speed dependent friction (multiplied by speed)
		// V2 = aerodynamic drag (multiplied by speed squared) 

		Vec3V m_LinearC;
		Vec3V m_LinearV;
		Vec3V m_LinearV2;
		Vec3V m_AngularC;
		Vec3V m_AngularV;
		Vec3V m_AngularV2;

	public:
		phArchetypeDamp()
		{

		}

		phArchetypeDamp(const datResource& rsc) : phArchetypePhys(rsc)
		{

		}
	};
}
