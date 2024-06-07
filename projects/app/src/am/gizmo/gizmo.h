//
// File: gizmo.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "gizmotranslation.h"
#include "gizmorotation.h"
#include "gizmoscale.h"

namespace rageam::gizmo
{
	inline bool Translate(ConstString id, Mat44V& inOutTransform) { return GIZMO_MANIPULATE(id, inOutTransform, GizmoTranslation); }
	inline bool Scale(ConstString id, Mat44V& inOutTransform)	  { return GIZMO_MANIPULATE(id, inOutTransform, GizmoScale); }
	inline bool Rotate(ConstString id, Mat44V& inOutTransform)	  { return GIZMO_MANIPULATE(id, inOutTransform, GizmoRotation); }
}
