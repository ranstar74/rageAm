//
// File: jointdata.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "rage/math/vecv.h"
#include "rage/paging/base.h"
#include "rage/paging/template/array.h"

namespace rage
{
	static constexpr int MAX_CONTROL_POINTS = 8;

	struct crControlPoint
	{
		float MaxSwing;
		float MinTwist;
		float MaxTwist;
	};

	struct crJointRotationLimit
	{
		int				BoneID;
		int				NumControlPoints;
		int				JointDOFs;
		Vec3V			ZeroRotationEulers;
		Vec3V			TwistAxis;
		float			TwistLimitMin;
		float			TwistLimitMax;
		float			SoftLimitScale;
		crControlPoint	ControlPoints[MAX_CONTROL_POINTS];
		bool			UseTwistLimits;
		bool			UseEulerAngles;
		bool			UsePerControlTwistLimits;
	};

	struct crJointScaleLimit
	{
		Vec3V LimitMin;
		Vec3V LimitMax;
	};

	struct crJointTranslationLimit
	{
		Vec3V LimitMin;
		Vec3V LimitMax;
	};

	class crJointData : pgBase
	{
		// TODO: This is wrong declaration from XML

		pgArray<crJointRotationLimit>		RotationLimits;
		pgArray<crJointScaleLimit>			TranslationLimits;
		pgArray<crJointTranslationLimit>	ScaleLimits;

		u32 m_RefCount = 1;

	public:
		crJointData()
		{
			
		}

		crJointData(const datResource& rsc)
		{
			
		}

		IMPLEMENT_REF_COUNTER(crJointData);
	};
}
