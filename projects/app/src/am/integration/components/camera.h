#pragma once

#ifdef AM_INTEGRATED

#include "am/integration/updatecomponent.h"
#include "am/integration/script/types.h"
#include "rage/math/mtxv.h"
#include "am/types.h"

namespace rageam::integration
{
	// TODO: Breaks if used inside a vehicle

	class ICameraComponent : public IUpdateComponent
	{
	protected:
		static inline ICameraComponent* sm_Active = nullptr;

	public:
		virtual const Vec3V&  GetFront() const = 0;
		virtual const Vec3V&  GetRight() const = 0;
		virtual const Vec3V&  GetUp() const = 0;
		virtual const Vec3V&  GetPosition() const = 0;
		virtual const Vec3V&  GetVelocity() = 0;
		virtual Mat44V		  GetMatrix() const = 0;
		virtual void          SetMatrix(const Mat44V& mtx) = 0;
		virtual void          SetPosition(const Vec3V& position) = 0;
		virtual void          LookAt(const Vec3V& point) = 0;

		virtual void DisableControls(bool) = 0;
		virtual bool ControlsDisabled() = 0;

		virtual void SetActive(bool active);
		bool         IsActive() const { return sm_Active == this; }

		static ICameraComponent* GetActiveCamera() { return sm_Active; }
	};
	using CameraComponent = ComponentOwner<ICameraComponent>;

	class CameraComponentBase : public ICameraComponent
	{
	protected:
		Vec3V          m_Right	= { 1, 0, 0 };
		Vec3V          m_Front	= { 0, 1, 0 };
		Vec3V          m_Up		= { 0, 0, 1 };
		Vec3V          m_Pos	= { 0, 0, 0 };
		Vec3V          m_OldPos = { 0, 0, 0 };
		Vec3V          m_Velocity;
		float          m_MouseFactor = 0.0018f;
		scrCameraIndex m_CameraHandle;
		scrBlipIndex   m_BlipHandle;
		pVoid          m_Camera = nullptr;
		bool           m_DisableControls = false;

	public:
		CameraComponentBase() = default;
		~CameraComponentBase() override;

		void OnStart() override;
		void OnEarlyUpdate() override;

		void          SetActive(bool active) override;
		const Vec3V&  GetFront() const override { return m_Front; }
		const Vec3V&  GetRight() const override { return m_Right; }
		const Vec3V&  GetUp() const override { return m_Up; }
		const Vec3V&  GetPosition() const override;
		const Vec3V&  GetVelocity() override;
		Mat44V        GetMatrix() const override;
		void          SetMatrix(const Mat44V& mtx) override;
		void          LookAt(const Vec3V& point) override;

		void DisableControls(bool value) override { m_DisableControls = value; }
		bool ControlsDisabled() override;
	};

	class FreeCamera : public CameraComponentBase
	{
		static constexpr float DEFAULT_MOVE_SPEED = 10.0f;

		// In meters per second
		ScalarV m_MinMoveSpeed = 0.01f;
		ScalarV m_MaxMoveSpeed = 900.0f;
		ScalarV m_MoveSpeed = DEFAULT_MOVE_SPEED;

	public:
		void OnEarlyUpdate() override;

		void SetPosition(const rage::Vec3V& position) override;
	};

	class OrbitCamera : public CameraComponentBase
	{
		Vec3V	m_Center;
		ScalarV	m_Radius;					// Distance from origin
		ScalarV	m_ShiftFactor = 0.3f;		// Panning speed
		ScalarV	m_MaxZoomDistance = 80.0f;

		void ComputeRadius();
	public:
		void OnEarlyUpdate() override;

		void LookAt(const rage::Vec3V& point) override;
		void SetPosition(const rage::Vec3V& position) override;

		void Rotate(float h, float v);
		void Zoom(const rage::ScalarV&);
	};
}
#endif
