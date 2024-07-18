//
// File: gizmobase.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#ifdef AM_INTEGRATED

#include "am/system/nullable.h"
#include "am/integration/gamedrawlists.h"
#include "am/graphics/color.h"
#include "helpers/propertystack.h"

#include <imgui.h>

// (=^-ω-^=) Gizmo system is separated on low level helper functions for drawing / hit testing
// and manager that's built on top of them
//
// This system is designed with flexebility in mind to add custom gizmos for manipulating
// complex data such as ladders, lights, audio, etc. in the future

namespace rageam::gizmo
{
	struct GizmoContext;

	// This is a small number because gizmo size is multiplied by distance to the camera
	// to make it look the same at any distance, we've choosen this approach
	// because it allows to easily find out the closest gizmo
	static const ScalarV GIZMO_SCALE = 1.0f / 6.5f;

	static constexpr u32 GIZMO_COLOR_WHITE = AM_COL32(140, 140, 140, 255);
	static constexpr u32 GIZMO_COLOR_RED = AM_COL32(200, 40, 10, 255);
	static constexpr u32 GIZMO_COLOR_GREEN = AM_COL32(95, 190, 5, 255);
	static constexpr u32 GIZMO_COLOR_BLUE = AM_COL32(0, 10, 255, 255);
	static constexpr u32 GIZMO_COLOR_CYAN = AM_COL32(75, 180, 250, 255);
	static constexpr u32 GIZMO_COLOR_YELLOW = AM_COL32(225, 220, 10, 255);
	static constexpr u32 GIZMO_COLOR_YELLOW2 = AM_COL32(225, 220, 10, 200);
	static constexpr u32 GIZMO_COLOR_HOVER = AM_COL32(255, 255, 0, 150);
	static constexpr u32 GIZMO_COLOR_GRAY = AM_COL32(130, 130, 130, 255);
	static constexpr u32 GIZMO_COLOR_GRAY2 = AM_COL32(130, 130, 130, 140);

	// Generic constants used in drawing and shape testing, however gizmos may define their own constants
	//
	// Translation Axes
	// - Axis:			The axis itself, represented by a line
	// - Cap:			Cone on the end of the axis
	// - Plane:			One of 3 planes (XY, XZ, YZ)
	// - Screen Plane:  Box in the center of the gizmo, like in 3Ds max, for moving in the screen space
	static constexpr int   AXIS_CAP_SEGMENTS = 8;
	static constexpr float AXIS_CAP_STEP = rage::PI2 / static_cast<float>(AXIS_CAP_SEGMENTS);
	static constexpr float AXIS_CAP_EXTENT = 0.2f;				// Length of the cone
	static const ScalarV   AXIS_CAP_EXTENTV = AXIS_CAP_EXTENT;
	static const ScalarV   AXIS_CAP_RADIUSV = 0.05f;			// Radius of the cone
	static const ScalarV   AXIS_CAP_HIT_RADIUSV = 0.075f;		// Defines radius of capsule for hit testing axes
	static const ScalarV   AXIS_CENTER_OFFSETV = 0.15f;			// To leave empty space in center of the gizmo for screen plane
	static const ScalarV   AXIS_PLANE_EXTENTV = 0.4f;			// Size of plane manipulator
	static const ScalarV   AXIS_SCREEN_PLANE_EXTENTV = 0.07f;	// Half size of the screen space plane manipulator
	// Rotation Axes
	static const ScalarV   ROT_CIRCLE_BORDER_WIDTH = 0.15f;		// Width of the circle border for hit test
	// Scale
	static const ScalarV   SCALE_BOX_EXTENTV = 0.07f;

	static auto& GetDrawList() { return integration::GameDrawLists::GetInstance()->Gizmo; }

	struct Ray { Vec3V Pos; Vec3V Dir; };

	// Those test functions are meant to be used with gizmos only, there's extra scale parameter to keep test results
	// consistent with graphical representation of the gizmo
	bool TestAxis(const Ray& ray, const Mat44V& transform, const Vec3V& normal, const ScalarV& scale, ScalarV& outDist);
	bool TestCapsule(const Ray& ray, const Mat44V& transform, const Vec3V& p1, const Vec3V& p2, const ScalarV& scale, ScalarV& outDist);
	bool TestPlane(const Ray& ray, const Mat44V& transform, const Vec3V& normal, const Vec3V& tangent, ScalarV& outDist);
	bool TestDisk(const Ray& ray, const Mat44V& transform, const Vec3V& normal, const ScalarV& scale, ScalarV& outDist);
	bool TestQuad(const Ray& ray, const Mat44V& transform, const Vec3V& p1, const Vec3V& p2, const Vec3V& p3, const Vec3V& p4, ScalarV& outDist);
	// Only tests if circle edge is intersected, excluding all area inside
	bool TestCircle(const Ray& ray, const Mat44V& transform, const Vec3V& normal, const ScalarV& scale, const ScalarV& borderWidth, ScalarV& outDist);

	void DrawCircleCameraCull(const Mat34V& camera, const Mat44V& transform, const rage::Vec3V& normal, const rage::Vec3V& tangent, u32 color);
	void DrawCircle(const Mat44V& transform, const rage::Vec3V& normal, const rage::Vec3V& tangent, const ScalarV& scale, u32 color);
	void DrawPlane(const Mat44V& transform, const Vec3V& normal, const Vec3V& tangent, u32 color1, u32 color2, bool hovered);
	void DrawQuad(const Mat44V& transform, const Vec3V& normal, const Vec3V& tangent, const ScalarV& extent, u32 color);
	void DrawAxis(const Mat44V& transform, const Vec3V& normal, const Vec3V& tangent, u32 color, bool hovered, bool cap = true);
	enum GizmoOrientation
	{
		GizmoLocal, // Fully preserves original matrix, both orientation and position
		GizmoWorld,	// Keeps only position
	};

	inline Mat44V ConvertTransformOrientation(const Mat44V& transform, GizmoOrientation orientation)
	{
		if (orientation == GizmoWorld)
			return Mat44V::Translation(transform.Pos);
		return transform; // GizmoLocal
	}

	inline Vec3V ConvertNormalOrientation(const Vec3V& normal, const Mat44V& transform, GizmoOrientation orientation)
	{
		if (orientation == GizmoWorld)
			return normal;
		return normal.TransformNormal(transform); // GizmoLocal
	}

	inline Vec2S GetMousePos()
	{
		// We use winapi call instead of ImGui::GetMousePos() because ImGui position is cached
		// and works incorretly with mouse wrapping (Gizmo::WrapCursor)
		POINT point = {};
		GetCursorPos(&point);
		return Vec2S(static_cast<float>(point.x), static_cast<float>(point.y));
	}

	enum GizmoConstraint
	{
		GizmoConstraint_None,
		GizmoConstraint_X,
		GizmoConstraint_Y,
		GizmoConstraint_Z,
	};

	// Defines normal of the plane to perform manipulation on
	// For example if we want to perform manipulation on AxisX, plane with Y axis as normal will be picked
	enum GizmoPlaneSelector
	{
		GizmoPlaneSelector_Invalid,
		GizmoPlaneSelector_AxisX,
		GizmoPlaneSelector_AxisY,
		GizmoPlaneSelector_AxisZ,
		GizmoPlaneSelector_View,		// Plane is aligned with camera view
		GizmoPlaneSelector_BestMatchX,	// Finds the most aligned plane to the camera
		GizmoPlaneSelector_BestMatchY,
		GizmoPlaneSelector_BestMatchZ,
	};

	// Used on translation axes to pick plane normal for performing translation itself,
	// it handles cases like picking best axis depending on view or straight returns camera front for view selector
	Vec3V SelectPlane(const Vec3V& cameraFront, GizmoPlaneSelector selector);
	// Calculates offset between points where given rays intersect on specified plane
	Vec3V ManipulateOnPlane(
		const Ray& startRay, const Ray& currentRay, const Mat44V& transform, const Mat34V& camera, 
		const Vec3V& planePos, const Vec3V& planeNormal, Vec3V& outFrom, Vec3V& outTo, GizmoConstraint constraint = GizmoConstraint_None);
	// Constraints point p2 on a line defined by p2 - p1, transform is used to move constraint axis to line space
	Vec3V ConstraintPoint(const Vec3V& p1, const Vec3V& p2, const Mat44V& transform, GizmoConstraint constraint);

	// Hash of the name string (however might be different value if other ID was pushed)
	using GizmoID = HashValue;
	// Abstract components of the gizmo.
	// Any gizmo can be composed out of multiple or single of following components,
	// only drawing and hit test detection is required
	using GizmoHandleID = int;
	static constexpr GizmoHandleID GizmoHandleID_Invalid = 0;

	// Result of shape test, with exact component that ray intersected (if any...)
	struct HitResult
	{
		ScalarV       Distance;
		GizmoHandleID Handle = 0;
	};

	/**
	 * \brief This is base gizmo class to handle all different types of gizmo.
	 * Instance of this class is created when gizmo is first drawn, and destroyed at the end of frame
	 * if it wasn't drawn (handled by GizmoManager).
	 * You can store additional parameters/state in the class itself.
	 */
	class Gizmo
	{
	public:
		virtual ~Gizmo() = default;
		// Called before perming hit test / drawing, could be used to pre-compute and cache geometry.
		// For example light gizmos are relatively complex and use lines for both drawing and hit test,
		// instead of computing lines twice, lines are cached
		virtual void Update(const GizmoContext& context) {}
		// Whether gizmo scale shouldn't be affected by camera position or distance
		// Internally gizmo size is scaled by distance to the camera
		virtual bool DynamicScale() const = 0;
		// Selection locks hit handle (using left mouse button)
		virtual bool CanSelect(GizmoHandleID handle) const { return false; }
		// Used on translation/rotation gizmos, for combining with other gizmos
		virtual bool TopMost() const { return false; }
		// Used on rotation gizmos to wrap cursor when doing manipulation
		virtual bool WrapCursor() const { return false; }
		// Invoked on first gizmo appearing on the screen
		virtual void OnStart(const GizmoContext& context) {}
		// Invoked if ESC or right mouse button was canceled
		// NOTE: Gizmo manager already returns initial (start) transform on canceling
		virtual void OnCancel(const GizmoContext& context) {}
		virtual void Draw(const GizmoContext& context) = 0;
		virtual void Manipulate(const GizmoContext& context, Vec3V& outOffset, Vec3V& outScale, QuatV& outRotation) = 0;
		// Transform is pre-converted to required orientation (world/local),
		//  scale is supplied additionally because in some cases it is faster/easier
		//  to use it instead of performing matrix multiplication
		// Returns all intersections (unsorted) of ray with gizmo components (handles)
		virtual List<HitResult> HitTest(const GizmoContext& context) = 0;
	};
	using GizmoUPtr = amUPtr<Gizmo>;
	// Delegate that creates gizmo, passed in manager functions
	using GizmoCreateFn = std::function<GizmoUPtr()>;

	/**
	 * \brief Gizmo type info declaration.
	 */
	struct GizmoInfo
	{
		GizmoCreateFn CreateFn;
		ConstString   DebugName;
		u32           NameHash; // Used to distinct gizmo types
	};

	/**
	 * \brief Record about created gizmo via the mangaer.
	 */
	struct GizmoEntry
	{
		GizmoID       ID;
		GizmoUPtr     Impl;
		GizmoInfo*	  Info;
		GizmoHandleID HitHandle;
		ScalarV       HitDistance;
		Mat44V        StartTransform; // Those 'start' ones are saved before doing manipulation
		Mat44V		  StartTransformOriented;
		Ray			  StartMouseRay;
		RECT		  StartMonitorWorkRect;
		Vec2S         StartMousePos;
		// Quite a hack, but for rotation gizmo we calculate rotation using
		// offset of mouse from starting position to current position,
		// but if we move mouse back (wrapping it), it will affect rotation
		Vec2S		  MousePosOffset;
		bool          DidHit;      // NOTE: This might be true for multiple gizmos
		bool          Manipulated; // Was this gizmo not only hovered but actually dragged?
		bool		  Canceled;
		int			  SelectedFrame; // Frame when gizmo was selected
		int			  UnselectedFrame; // Frame when gizmo was unselected
		int           LastFrame;   // Used to remove old gizmos that weren't drawn more than one frame

		Vec2S GetMousePos() const;
		Vec2S GetMouseDelta() const { return StartMousePos - GetMousePos(); }

		void DoCancel(const GizmoContext& context)
		{
			Manipulated = false;
			Canceled = true;
			Impl->OnCancel(context);
		}
	};

	struct GizmoContext
	{
		pVoid			 UserData;
		Mat34V           Camera;
		Mat44V           Transform; // Orientation is pre-applied
		ScalarV          Scale;
		Ray              MouseRay;
		bool             DebugInfo;
		GizmoOrientation Orientation;
		Vec2S			 MouseDelta; // Only valid during manipulation
		// Fields below are only valid in Gizmo::Manipulate
		GizmoHandleID	 SelectedHandle;
		// Extra ID to make hovering other handles of selected gizmo possible
		// To be used with only non-selectable handles
		GizmoHandleID	 HoveredHandle;
		Mat44V			 StartTransform; // Orientation is not applied!
		Mat44V			 StartTransformOriented;
		Ray				 StartMouseRay;
		bool			 IsSelected;

		template<typename T>
		T& GetUserData() const { AM_ASSERTS(UserData); return *static_cast<T*>(UserData); }

		bool IsHandleSelectedOrHovered(GizmoHandleID handle) const
		{
			return SelectedHandle == handle || HoveredHandle == handle;
		}
	};

	struct GizmoState
	{
		bool Manipulated;
		bool Hovered;
		bool Selected;
		bool JustSelected;
		bool JustUnselected;
		GizmoHandleID SelectedHandle;
	};

	/**
	 * \brief This class allows to create multiple gizmos across the scene.
	 * It holds all the hit tests and allows to get nearest hovered handle to the camera.
	 */
	class GizmoManager : public Singleton<GizmoManager>
	{
		List<GizmoID>       m_IDStack;
		HashSet<GizmoEntry> m_Gizmos;
		Nullable<GizmoID>   m_ClosestHoveredGizmo;
		Ray                 m_MouseRay;
		Mat34V              m_Camera;
		Nullable<GizmoID>   m_SelectedID;
		GizmoHandleID		m_SelectedHandle = GizmoHandleID_Invalid;

		GizmoEntry& ManipulateInternal(GizmoID id, Mat44V& inOutTransform, GizmoInfo* info, pVoid userData, ConstString parentID);

	public:
		GizmoManager();

		IMPLEMENT_PROPERTY_STACK(bool, CanSelect, true);

		GizmoID GetID(ConstString stringID) const;
		void PushID(ConstString id);
		void PopID();

		bool CanInteract() const;

		void BeginFrame();
		void EndFrame(); // Removes unused gizmos and finds closest hovered gizmo to the camera

		void SetSelectedID(ConstString id);
		void ResetSelection();

		// This function automatically manages gizmo's lifetime, including drawing and manipulation
		// state is out parameter with lot of useful information about the gizmo
		// userData can be accessed inside gizmo using Gizmo::GetUserData
		// parentID specifies gizmo linked to this one, it is used to cancel hierarchhy of gizmos, for example when light gizmo creates sub gizmo for manipulating target point
		void Manipulate(ConstString id, Mat44V& inOutTransform, GizmoInfo* info, GizmoState& state, pVoid userData = nullptr, ConstString parentID = nullptr);
		bool Manipulate(ConstString id, Mat44V& inOutTransform, GizmoInfo* info, bool* outIsHovered = nullptr, pVoid userData = nullptr, ConstString parentID = nullptr);
		bool ManipulateDefault(ConstString id, Mat44V& inOutTransform, pVoid userData = nullptr, ConstString parentID = nullptr)
		{
			AM_ASSERT(DefaultInfo != nullptr, "GizmoManager::ManipulateDefault() -> Default gizmo info is not set!");
			return Manipulate(id, inOutTransform, DefaultInfo, nullptr, userData, parentID);
		}

		GizmoOrientation Orientation = GizmoLocal;
		// Shows extra debug information on gizmos
		bool DebugInfo = false;
		// This allows to change gizmo type globally
		GizmoInfo* DefaultInfo = nullptr;
	};
}

// Gizmo info must be declared in header and initialized in source
#define GIZMO_DECLARE_INFO static rageam::gizmo::GizmoInfo Info
#define GIZMO_INITIALIZE_INFO(type, name)					\
	rageam::gizmo::GizmoInfo type::Info =			\
		{													\
			[] { return std::make_unique<type>(); },		\
			name,											\
			Hash(name)										\
		}

#define GIZMO_MANIPULATE(id, transform, type) \
	rageam::gizmo::GizmoManager::GetInstance()->Manipulate(id, transform, &type::Info)

#define GIZMO_MANIPULATE_EX(id, transform, type, userData)	\
	rageam::gizmo::GizmoManager::GetInstance()->Manipulate(id, transform, &type::Info, nullptr, userData)

#define GIZMO rageam::gizmo::GizmoManager::GetInstance()

#define GIZMO_GET_INFO(type) &type::Info

#endif
