#include "camera.h"

#ifdef AM_INTEGRATED

#include "am/integration/memory/address.h"
#include "am/integration/memory/hook.h"
#include "am/integration/im3d.h"
#include "am/integration/script/core.h"
#include "am/integration/script/extensions.h"
#include "rage/framework/pool.h"
#include "imgui_internal.h"

void rageam::integration::ICameraComponent::SetActive(bool active)
{
	if (active) sm_Active = this;
	else sm_Active = nullptr;
}

rageam::integration::CameraComponentBase::~CameraComponentBase()
{
	if (IsActive())
	{
		CameraComponentBase::SetActive(false);
	}
	scrDestroyCam(m_CameraHandle);
}

void rageam::integration::CameraComponentBase::OnStart()
{
	m_CameraHandle = scrCreateCamWithParams("DEFAULT_SCRIPTED_CAMERA", scrVector(), scrVector());

	// Obtain camera resource pointer from pool
	static rage::fwBasePool* camPool = *gmAddress::Scan(
#if APP_BUILD_2699_16_RELEASE_NO_OPT
		"48 89 54 24 10 89 4C 24 08 48 83 EC 48 83 7C 24 50 00 7C 21")
	                                    .GetAt(0x14)
	                                    .GetRef(3)
#else
		"48 8B 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B C8 EB 02 33 C9 48 85 C9 74 26")
	                                    .GetRef(3)
#endif
	                                    .To<rage::fwBasePool**>();

	m_Camera = camPool->GetSlot(POOL_TO_GUID(m_CameraHandle));

	AM_ASSERT(m_Camera != nullptr, "CameraComponentBase::EarlyUpdate() -> Failed to create game camera.");

	// Camera was set to active before it was created, make sure it's active now
	if (IsActive())
		SetActive(true);
}

void rageam::integration::CameraComponentBase::OnUpdate()
{
	// Update only if position has changed
	if (m_OldPos != m_Pos)
	{
		m_Velocity = m_Pos - m_OldPos;
		m_OldPos = m_Pos;

		// Load map around camera
		scrSetFocusPosAndVel(m_Pos, m_Velocity);

		// Update camera
		if (m_Camera)
		{
			u64 frame = (u64)m_Camera + 0x20;
			*(rage::Mat44V*)(frame + 0x10) = GetMatrix();
		}
	}

	// Update blip
	if (m_BlipHandle.IsValid())
	{
		int rotation = scrRound(scrGetHeadingFromVector2D(m_Front.X(), m_Front.Y()));
		scrSetBlipCoords(m_BlipHandle, m_Pos);
		scrSetBlipRotation(m_BlipHandle, rotation);
		scrLockMinimapAngle(rotation);
		scrLockMinimapPosition(m_Pos.X(), m_Pos.Y());
	}
}

void rageam::integration::CameraComponentBase::SetActive(bool active)
{
	ICameraComponent::SetActive(active);

	if (active)
	{
		scrSetCamActive(m_CameraHandle, true);
		scrRenderScriptCams(true, false);
		scrCascadeShadowsSetCascadeBoundsScale(2.0f);
		scrCascadeShadowsSetAircraftMode(true);
		if(!m_BlipHandle.IsValid())
		{
			m_BlipHandle = scrAddBlipForCoord(scrVector());
			scrSetBlipSprite(m_BlipHandle, 6); // Arrow
			scrSetBlipColour(m_BlipHandle, 5); // Yellow
			scrSetBlipFlashes(m_BlipHandle, true);
			scrSetBlipFlashInterval(m_BlipHandle, 1000);
			scrSetBlipScale(m_BlipHandle, 0.7f);
		}
	}
	else
	{
		// Before disabling rendering cams we have to ensure that this camera is still rendering,
		// if we instantly enable another camera after disabling one, another camera shouldn't be disabled
		if (scrIsCamActive(m_CameraHandle))
		{
			scrRenderScriptCams(false, false);
			scrClearFocus();
			scrCascadeShadowsInitSession();
			scrUnlockMinimapAngle();
			scrUnlockMinimapPosition();
		}
		scrRemoveBlip(m_BlipHandle);
	}
}

const rage::Vec3V& rageam::integration::CameraComponentBase::GetPosition() const
{
	return m_Pos;
}

void rageam::integration::CameraComponentBase::SetMatrix(const rage::Mat44V& mtx)
{
	m_Right = mtx.Right;
	m_Front = mtx.Front;
	m_Up = mtx.Up;
	m_Pos = mtx.Pos;
}

rage::Mat44V rageam::integration::CameraComponentBase::GetMatrix() const
{
	rage::Mat34V mtx;
	mtx.Right = m_Right;
	mtx.Front = m_Front;
	mtx.Up = m_Up;
	mtx.Pos = m_Pos;
	return mtx.To44();
}

const rage::Vec3V& rageam::integration::CameraComponentBase::GetVelocity()
{
	return m_Velocity;
}

void rageam::integration::CameraComponentBase::LookAt(const rage::Vec3V& point)
{
	m_Front = (point - m_Pos).Normalized();;
	m_Right = m_Front.Cross(rage::VEC_UP).Normalized();
	m_Up = m_Right.Cross(m_Front).Normalized();
}

bool rageam::integration::CameraComponentBase::ControlsDisabled()
{
	// Don't process camera controls if we're hovering window or gizmo
	return m_DisableControls || !Im3D::IsViewportHovered();
}

void rageam::integration::FreeCamera::OnUpdate()
{
	if (!IsActive())
		return;

	CameraComponentBase::OnUpdate();

	if (ControlsDisabled())
		return;

	// Speed edit by zooming
	rage::ScalarV scrollWheel = ImGui::GetKeyData(ImGuiKey_MouseWheelY)->AnalogValue;
	if (!scrollWheel.AlmostEqual(rage::S_EPSION))
	{
		const rage::ScalarV zoomFactorIn = { -0.15f };
		const rage::ScalarV zoomFactorOut = { 0.25f };

		if (scrollWheel < rage::S_ZERO)
			m_MoveSpeed += m_MoveSpeed * zoomFactorIn;
		else
			m_MoveSpeed += m_MoveSpeed * zoomFactorOut;

		m_MoveSpeed = m_MoveSpeed.Clamp(m_MinMoveSpeed, m_MaxMoveSpeed);
	}

	if (ImGui::IsKeyPressed(ImGuiKey_LeftBracket, false))
	{
		m_MoveSpeed = DEFAULT_MOVE_SPEED;
	}

	// Movement
	rage::Vec3V move = rage::S_ZERO;
	move += m_Front * (ImGui::GetKeyData(ImGuiKey_W)->AnalogValue - ImGui::GetKeyData(ImGuiKey_S)->AnalogValue);
	move += m_Right * (ImGui::GetKeyData(ImGuiKey_D)->AnalogValue - ImGui::GetKeyData(ImGuiKey_A)->AnalogValue);
	rage::ScalarV moveMag = move.LengthSquared();
	if (moveMag > rage::S_EPSION) // Check if we have any input at all
	{
		rage::ScalarV moveSpeed = m_MoveSpeed;

		const rage::ScalarV boostFactorUp = { 2.0f };
		const rage::ScalarV boostFactorDown = { -0.5f };
		if (ImGui::IsKeyDown(ImGuiKey_LeftShift))
			moveSpeed += moveSpeed * boostFactorUp;
		if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl))
			moveSpeed += moveSpeed * boostFactorDown;

		move *= moveMag.ReciprocalSqrt(); // Normalize
		move *= ImGui::GetIO().DeltaTime;
		move *= moveSpeed;

		m_Pos += move;
	}

	// Rotation
	static ImVec2 prevMousePos = {};
	ImVec2 mousePos = ImGui::GetMousePos();
	ImVec2 mouseDelta = mousePos - prevMousePos;
	if (ImGui::IsKeyDown(ImGuiKey_MouseMiddle))
	{
		scrDisableAllControlsThisFrame();

		float deltaX = mouseDelta.x;
		float deltaY = mouseDelta.y;

		rage::QuatV rotVertical = DirectX::XMQuaternionRotationNormal(m_Right, m_MouseFactor * -deltaY);
		rage::QuatV rotHorizontal = DirectX::XMQuaternionRotationNormal(rage::VEC_UP, m_MouseFactor * -deltaX);

		m_Front = DirectX::XMVector3Rotate(m_Front, rotVertical);
		m_Front = DirectX::XMVector3Rotate(m_Front, rotHorizontal);
		m_Right = DirectX::XMVector3Rotate(m_Right, rotVertical);
		m_Right = DirectX::XMVector3Rotate(m_Right, rotHorizontal);
		m_Up = m_Right.Cross(m_Front).Normalized();
	}
	prevMousePos = mousePos;
}

void rageam::integration::FreeCamera::SetPosition(const rage::Vec3V& position)
{
	m_Pos = position;
}

void rageam::integration::OrbitCamera::ComputeRadius()
{
	m_Radius = (m_Pos - m_Center).LengthEstimate();
}

void rageam::integration::OrbitCamera::OnUpdate()
{
	if (!IsActive())
		return;

	CameraComponentBase::OnUpdate();

	if (ControlsDisabled())
		return;

	// Shifting / zooming distance must scale with distance from origin
	rage::ScalarV panFactor = m_Radius * m_ShiftFactor;

	// Zooming
	rage::ScalarV scrollWheel = ImGui::GetKeyData(ImGuiKey_MouseWheelY)->AnalogValue;
	if (!scrollWheel.AlmostEqual(rage::S_EPSION))
		Zoom(scrollWheel * panFactor);

	if (ImGui::IsKeyPressed(ImGuiKey_KeypadAdd))
		Zoom(panFactor);

	if (ImGui::IsKeyPressed(ImGuiKey_KeypadSubtract))
		Zoom(-panFactor);

	static ImVec2 prevMousePos = {};
	ImVec2 mousePos = ImGui::GetMousePos();
	ImVec2 mouseDelta = mousePos - prevMousePos;
	float deltaX = -mouseDelta.x * m_MouseFactor;
	float deltaY = -mouseDelta.y * m_MouseFactor;
	prevMousePos = mousePos;

	// Moving
	if (ImGui::IsKeyDown(ImGuiKey_MouseMiddle) && ImGui::IsKeyDown(ImGuiKey_LeftShift))
	{
		rage::ScalarV moveX = panFactor * deltaX;
		rage::ScalarV moveY = panFactor * -deltaY;
		rage::Vec3V shift = m_Right * moveX + m_Up * moveY;

		m_Center += shift;
		m_Pos += shift;

		ComputeRadius();
	}
	// Rotation
	else if (ImGui::IsKeyDown(ImGuiKey_MouseMiddle))
	{
		scrDisableAllControlsThisFrame();

		Rotate(deltaX, deltaY);
	}

	// Center camera around clicked position in world
	static scrShapetestIndex s_TestHandle;
	if (!s_TestHandle.IsValid() && ImGui::IsKeyDown(ImGuiKey_LeftAlt) && ImGui::IsKeyPressed(ImGuiKey_MouseMiddle, false))
	{
		scrVector startPos, endPos;
		s_TestHandle = scrStartShapeTestMouseCursorLosProbe(startPos, endPos, SCRIPT_INCLUDE_ALL);
	}

	// Try to get async shape test result
	if (s_TestHandle.IsValid())
	{
		bool hit;
		scrVector hitPos, hitNormal;
		scrEntityIndex hitEntity;
		scrShapetestStatus shapeStatus = scrGetShapeTestResult(s_TestHandle, hit, hitPos, hitNormal, hitEntity);
		if (shapeStatus == SHAPETEST_STATUS_NONEXISTENT)
		{
			s_TestHandle.Reset();
		}
		else if (shapeStatus == SHAPETEST_STATUS_RESULTS_READY && hit)
		{
			LookAt(hitPos);
			s_TestHandle.Reset();
		}
	}
}

void rageam::integration::OrbitCamera::LookAt(const rage::Vec3V& point)
{
	CameraComponentBase::LookAt(point);
	m_Center = point;
	ComputeRadius();
}

void rageam::integration::OrbitCamera::SetPosition(const rage::Vec3V& position)
{
	m_Pos = position;
	LookAt(m_Center);
}

void rageam::integration::OrbitCamera::Rotate(float h, float v)
{
	rage::QuatV rotHorizontal = DirectX::XMQuaternionRotationNormal(rage::VEC_UP, h);
	rage::QuatV rotVertical = DirectX::XMQuaternionRotationNormal(m_Right, v);

	// Rotate camera position around center
	rage::Vec3V oldOffset = m_Pos - m_Center;
	rage::Vec3V newOffset = DirectX::XMVector3Rotate(oldOffset, rotVertical);
	newOffset = DirectX::XMVector3Rotate(newOffset, rotHorizontal);

	// We must ensure that after zooming we won't zoom through center
	if (newOffset.Dot(oldOffset) >= rage::S_ZERO)
	{
		m_Pos = m_Center + newOffset;
		LookAt(m_Center);
	}
}

void rageam::integration::OrbitCamera::Zoom(const rage::ScalarV& d)
{
	rage::ScalarV zoomDistance = d.Clamp(-m_MaxZoomDistance, m_MaxZoomDistance);
	m_Pos += m_Front * zoomDistance;
	ComputeRadius();
}

#endif
