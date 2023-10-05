#pragma once
#ifdef AM_INTEGRATED

#include "am/integration/updatecomponent.h"
#include "rage/math/mtxv.h"

namespace rageam::integration
{
	// TODO: Breaks if used inside a vehicle

	class ICameraComponent : public IUpdateComponent
	{
	protected:
		static inline ICameraComponent* sm_Active = nullptr;

	public:

		virtual const rage::Vec3V& GetFront() const = 0;
		virtual const rage::Vec3V& GetRight() const = 0;
		virtual const rage::Vec3V& GetUp() const = 0;
		virtual const rage::Vec3V& GetPosition() const = 0;
		virtual const rage::Vec3V& GetVelocity() = 0;
		virtual const rage::Mat44V& GetMatrix() const = 0;
		virtual void SetMatrix(const rage::Mat44V& mtx) = 0;
		virtual void SetPosition(const rage::Vec3V& position) = 0;
		virtual void LookAt(const rage::Vec3V& point) = 0;

		virtual void DisableControls(bool) = 0;

		virtual void SetActive(bool active);
		bool IsActive() const { return sm_Active == this; }

		static ICameraComponent* GetActiveCamera() { return sm_Active; }
	};

	class CameraComponentBase : public ICameraComponent
	{
	protected:
		rage::Vec3V			m_Right = { 1, 0, 0 };
		rage::Vec3V			m_Front = { 0, 1, 0 };
		rage::Vec3V			m_Up = { 0, 0, 1 };
		rage::Vec3V			m_Pos = { 0, 0, 0 };
		rage::Vec3V			m_OldPos;
		rage::Vec3V			m_Velocity;
		float				m_MouseFactor = 0.0018f;
		int					m_CameraHandle = -1;
		int					m_BlipHandle = 0;
		pVoid				m_Camera = nullptr;
		std::atomic_bool	m_DisableControls = false;

		static void EnableCameraAsync(int camera);
		static void DisableCameraAsync(int camera);
		static void DestroyCameraAsync(int camera);
		static void IncreaseShadowRenderDistance();
		static void ResetShadowRenderDistance();
		static void DestroyBlipAsync(int blip);
		void CreateBlipAsync();

	public:
		CameraComponentBase() = default;

		bool OnAbort() override;
		void OnStart() override;
		void OnEarlyUpdate() override;

		void SetActive(bool active) override;

		const rage::Vec3V& GetFront() const override { return m_Front; }
		const rage::Vec3V& GetRight() const override { return m_Right; }
		const rage::Vec3V& GetUp() const override { return m_Up; }
		const rage::Vec3V& GetPosition() const override;
		const rage::Vec3V& GetVelocity() override;
		const rage::Mat44V& GetMatrix() const override;
		void SetMatrix(const rage::Mat44V& mtx) override;
		void LookAt(const rage::Vec3V& point) override;

		void DisableControls(bool value) override { m_DisableControls = value; }
	};

	class FreeCamera : public CameraComponentBase
	{
		static constexpr float DEFAULT_MOVE_SPEED = 10.0f;

		// In meters per second
		rage::ScalarV m_MinMoveSpeed = 0.01f;
		rage::ScalarV m_MaxMoveSpeed = 900.0f;
		rage::ScalarV m_MoveSpeed = DEFAULT_MOVE_SPEED;

	public:
		void OnEarlyUpdate() override;

		void SetPosition(const rage::Vec3V& position) override;
	};

	class OrbitCamera : public CameraComponentBase
	{
		rage::Vec3V		m_Center;
		rage::ScalarV	m_Radius;				// Distance from origin
		rage::ScalarV	m_ShiftFactor = 0.3f;	// Panning speed
		rage::ScalarV	m_MaxZoomDistance = 80.0f;

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
