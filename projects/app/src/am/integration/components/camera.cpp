#include "camera.h"

#ifdef AM_INTEGRATED

#include "imgui_internal.h"
#include "am/integration/hooks/gameinput.h"
#include "am/integration/memory/address.h"
#include "am/integration/memory/hook.h"
#include "rage/framework/pool.h"
#include "am/integration/im3d.h"

void rageam::integration::ICameraComponent::SetActive(bool active)
{
	if (active)
		sm_Active = this;
	else
		sm_Active = nullptr;
}

// Those function are called from destructor in any thread
// so we have to dispatch it to script thread

void rageam::integration::CameraComponentBase::EnableCameraAsync(int camera)
{
	//scrInvoke([=]
	//	{
	//		//SHV::CAM::SET_CAM_ACTIVE(camera, TRUE);
	//		//SHV::CAM::RENDER_SCRIPT_CAMS(TRUE, FALSE, 3000, TRUE, 0);
	//		IncreaseShadowRenderDistance();
	//	});
}

void rageam::integration::CameraComponentBase::DisableCameraAsync(int camera)
{
	//scrInvoke([=]
	//	{
	//		// Before disabling rendering cams we have to ensure that this camera is still rendering,
	//		// if we instantly enable another camera after disabling one, another camera shouldn't be disabled
	//		//if (SHV::CAM::IS_CAM_ACTIVE(camera))
	//		//{
	//			//SHV::CAM::RENDER_SCRIPT_CAMS(FALSE, FALSE, 3000, TRUE, 0);
	//			//SHV::STREAMING::CLEAR_FOCUS();
	//			//ResetShadowRenderDistance();
	//		//}
	//	});
}

void rageam::integration::CameraComponentBase::DestroyCameraAsync(int camera)
{
	//scrInvoke([=]
	//	{
	//		// SHV::CAM::DESTROY_CAM(camera, FALSE);
	//	});
}

void rageam::integration::CameraComponentBase::IncreaseShadowRenderDistance()
{
	//SHV::GRAPHICS::CASCADE_SHADOWS_SET_CASCADE_BOUNDS_SCALE(2.0f);
	//SHV::GRAPHICS::CASCADE_SHADOWS_SET_AIRCRAFT_MODE(TRUE);
}

void rageam::integration::CameraComponentBase::ResetShadowRenderDistance()
{
	//SHV::GRAPHICS::CASCADE_SHADOWS_INIT_SESSION();
}

void rageam::integration::CameraComponentBase::DestroyBlipAsync(int blip)
{
	//scrInvoke([=]
	//	{
	//		int blipIndex = blip;
	//		//SHV::UI::REMOVE_BLIP(&blipIndex);
	//	});
}

void rageam::integration::CameraComponentBase::CreateBlipAsync()
{
	//scrInvoke([&]
	//	{
	//		//if (m_BlipHandle != 0)
	//		//	SHV::UI::REMOVE_BLIP(&m_BlipHandle);

	//		//m_BlipHandle = SHV::UI::ADD_BLIP_FOR_COORD(0, 0, 0);
	//		//SHV::UI::SET_BLIP_SPRITE(m_BlipHandle, 6); // Arrow
	//		//SHV::UI::SET_BLIP_COLOUR(m_BlipHandle, 5); // Yellow
	//		//SHV::UI::SET_BLIP_FLASHES(m_BlipHandle, TRUE);
	//		//SHV::UI::SET_BLIP_FLASH_INTERVAL(m_BlipHandle, 1000);
	//		//SHV::UI::SET_BLIP_SCALE(m_BlipHandle, 0.7f);
	//	});
}

bool rageam::integration::CameraComponentBase::OnAbort()
{
	if (IsActive())
	{
		CameraComponentBase::SetActive(false);

		//SHV::UI::UNLOCK_MINIMAP_ANGLE();
		//SHV::UI::UNLOCK_MINIMAP_POSITION();
	}

	DestroyBlipAsync(m_BlipHandle);
	DestroyCameraAsync(m_CameraHandle);

	return true;
}

void rageam::integration::CameraComponentBase::OnStart()
{
	char name[] = "DEFAULT_SCRIPTED_CAMERA";
	//m_CameraHandle = SHV::CAM::CREATE_CAM_WITH_PARAMS(
	//	name, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 65.0f, FALSE, 2);

	// Obtain camera resource pointer from pool
	static rage::fwBasePool* camPool = *gmAddress::Scan(
		"48 8B 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B C8 EB 02 33 C9 48 85 C9 74 26")
		.GetRef(3)
		.To<rage::fwBasePool**>();

	m_Camera = camPool->GetSlot(POOL_TO_GUID(m_CameraHandle));

	AM_ASSERT(m_Camera != nullptr, "CameraComponentBase::EarlyUpdate() -> Failed to create game camera.");

	// Camera was set to active before it was created, make sure it's active now
	if (IsActive())
		SetActive(true);
}

void rageam::integration::CameraComponentBase::OnEarlyUpdate()
{
	m_Velocity = m_Pos - m_OldPos;
	m_OldPos = m_Pos;

	// Load map around camera
	//SHV::STREAMING::SET_FOCUS_POS_AND_VEL(
	//	m_Pos.X(), m_Pos.Y(), m_Pos.Z(), m_Velocity.X(), m_Velocity.Y(), m_Velocity.Z());

	// Update camera
	if (m_Camera)
	{
		u64 frame = (u64)m_Camera + 0x20;
		*(rage::Mat44V*)(frame + 0x10) = GetMatrix();
	}

	// Update blip
	if (m_BlipHandle != 0)
	{
		//int rotation = SHV::SYSTEM::ROUND(SHV::GAMEPLAY::GET_HEADING_FROM_VECTOR_2D(m_Front.X(), m_Front.Y()));
		//SHV::UI::SET_BLIP_COORDS(m_BlipHandle, m_Pos.X(), m_Pos.Y(), m_Pos.Z());
		//SHV::UI::SET_BLIP_ROTATION(m_BlipHandle, rotation);
		//SHV::UI::LOCK_MINIMAP_ANGLE(rotation);
		//SHV::UI::LOCK_MINIMAP_POSITION(m_Pos.X(), m_Pos.Y());
	}
}

void rageam::integration::CameraComponentBase::SetActive(bool active)
{
	ICameraComponent::SetActive(active);

	// This function can be called from any thread, dispatch call to script thread

	if (active)
	{
		EnableCameraAsync(m_CameraHandle);
		CreateBlipAsync();
	}
	else
	{
		DisableCameraAsync(m_CameraHandle);
		DestroyBlipAsync(m_BlipHandle);
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

const rage::Mat44V& rageam::integration::CameraComponentBase::GetMatrix() const
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

void rageam::integration::FreeCamera::OnEarlyUpdate()
{
	//if (!IsActive())
	//	return;

	//CameraComponentBase::OnEarlyUpdate();

	//if (ControlsDisabled())
	//	return;

	//// Speed edit by zooming
	//rage::ScalarV scrollWheel = ImGui::GetKeyData(ImGuiKey_MouseWheelY)->AnalogValue;
	//if (!scrollWheel.AlmostEqual(rage::S_EPSION))
	//{
	//	const rage::ScalarV zoomFactorIn = { -0.15f };
	//	const rage::ScalarV zoomFactorOut = { 0.25f };

	//	if (scrollWheel < rage::S_ZERO)
	//		m_MoveSpeed += m_MoveSpeed * zoomFactorIn;
	//	else
	//		m_MoveSpeed += m_MoveSpeed * zoomFactorOut;

	//	m_MoveSpeed = m_MoveSpeed.Clamp(m_MinMoveSpeed, m_MaxMoveSpeed);
	//}

	//if (ImGui::IsKeyPressed(ImGuiKey_LeftBracket, false))
	//{
	//	m_MoveSpeed = DEFAULT_MOVE_SPEED;
	//}

	//// Movement
	//rage::Vec3V move = rage::S_ZERO;
	//move += m_Front * (ImGui::GetKeyData(ImGuiKey_W)->AnalogValue - ImGui::GetKeyData(ImGuiKey_S)->AnalogValue);
	//move += m_Right * (ImGui::GetKeyData(ImGuiKey_D)->AnalogValue - ImGui::GetKeyData(ImGuiKey_A)->AnalogValue);
	//rage::ScalarV moveMag = move.LengthSquared();
	//if (moveMag > rage::S_EPSION) // Check if we have any input at all
	//{
	//	rage::ScalarV moveSpeed = m_MoveSpeed;

	//	const rage::ScalarV boostFactorUp = { 2.0f };
	//	const rage::ScalarV boostFactorDown = { -0.5f };
	//	if (ImGui::IsKeyDown(ImGuiKey_LeftShift))
	//		moveSpeed += moveSpeed * boostFactorUp;
	//	if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl))
	//		moveSpeed += moveSpeed * boostFactorDown;

	//	move *= moveMag.ReciprocalSqrt(); // Normalize
	//	move *= ImGui::GetIO().DeltaTime;
	//	move *= moveSpeed;

	//	m_Pos += move;
	//}

	//// Rotation
	//if (ImGui::IsKeyDown(ImGuiKey_MouseMiddle))
	//{
	//	hooks::GameInput::DisableAllControlsThisFrame();

	//	InputState& input = Gui->Input.State;
	//	float deltaX = input.Horizontal;
	//	float deltaY = input.Vertical;

	//	rage::QuatV rotVertical = DirectX::XMQuaternionRotationNormal(m_Right, m_MouseFactor * -deltaY);
	//	rage::QuatV rotHorizontal = DirectX::XMQuaternionRotationNormal(rage::VEC_UP, m_MouseFactor * -deltaX);

	//	m_Front = DirectX::XMVector3Rotate(m_Front, rotVertical);
	//	m_Front = DirectX::XMVector3Rotate(m_Front, rotHorizontal);
	//	m_Right = DirectX::XMVector3Rotate(m_Right, rotVertical);
	//	m_Right = DirectX::XMVector3Rotate(m_Right, rotHorizontal);
	//	m_Up = m_Right.Cross(m_Front).Normalized();
	//}
}

void rageam::integration::FreeCamera::SetPosition(const rage::Vec3V& position)
{
	m_Pos = position;
}

void rageam::integration::OrbitCamera::ComputeRadius()
{
	m_Radius = (m_Pos - m_Center).LengthEstimate();
}

void rageam::integration::OrbitCamera::OnEarlyUpdate()
{
	//if (!IsActive())
	//	return;

	//CameraComponentBase::OnEarlyUpdate();

	//if (ControlsDisabled())
	//	return;

	//// Shifting / zooming distance must scale with distance from origin
	//rage::ScalarV panFactor = m_Radius * m_ShiftFactor;

	//// Zooming
	//rage::ScalarV scrollWheel = ImGui::GetKeyData(ImGuiKey_MouseWheelY)->AnalogValue;
	//if (!scrollWheel.AlmostEqual(rage::S_EPSION))
	//	Zoom(scrollWheel * panFactor);

	//if (ImGui::IsKeyPressed(ImGuiKey_KeypadAdd))
	//	Zoom(panFactor);

	//if (ImGui::IsKeyPressed(ImGuiKey_KeypadSubtract))
	//	Zoom(-panFactor);

	//InputState& input = Gui->Input.State; // TODO: Move input from Gui?
	//float deltaX = -input.Horizontal * m_MouseFactor;
	//float deltaY = -input.Vertical * m_MouseFactor;

	//// Moving
	//if (ImGui::IsKeyDown(ImGuiKey_MouseMiddle) && ImGui::IsKeyDown(ImGuiKey_LeftShift))
	//{
	//	rage::ScalarV moveX = panFactor * deltaX;
	//	rage::ScalarV moveY = panFactor * -deltaY;
	//	rage::Vec3V shift = m_Right * moveX + m_Up * moveY;

	//	m_Center += shift;
	//	m_Pos += shift;

	//	ComputeRadius();
	//}
	//// Rotation
	//else if (ImGui::IsKeyDown(ImGuiKey_MouseMiddle))
	//{
	//	hooks::GameInput::DisableAllControlsThisFrame();

	//	Rotate(deltaX, deltaY);
	//}

	//// Center camera around clicked position in world
	//static int s_TestHandle = 0;
	//if (s_TestHandle == 0 && ImGui::IsKeyDown(ImGuiKey_LeftAlt) && ImGui::IsKeyPressed(ImGuiKey_MouseMiddle, false))
	//{
	//	SHV::Vector3 startPos;
	//	SHV::Vector3 endPos;
	//	s_TestHandle = SHV::WORLDPROBE::START_SHAPE_TEST_MOUSE_CURSOR_LOS_PROBE(
	//		&startPos, &endPos, 511, NULL, 7);
	//}

	//// Try to get async shape test result
	//if (s_TestHandle != 0)
	//{
	//	BOOL hit;
	//	SHV::Vector3 hitPos;
	//	SHV::Vector3 hitNormal;
	//	SHV::Entity hitEntity;

	//	int shapeStatus = SHV::WORLDPROBE::GET_SHAPE_TEST_RESULT(s_TestHandle, &hit, &hitPos, &hitNormal, &hitEntity);
	//	if (shapeStatus == 0) // Canceled
	//	{
	//		s_TestHandle = 0;
	//	}
	//	else if (shapeStatus == 2 && hit) // Ready & Hit
	//	{
	//		LookAt(rage::Vec3V(hitPos.x, hitPos.y, hitPos.z));
	//		s_TestHandle = 0;
	//	}
	//}
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
