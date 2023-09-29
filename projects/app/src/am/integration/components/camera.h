#pragma once
#ifdef AM_INTEGRATED

#include "am/integration/updatecomponent.h"
#include "rage/math/mtxv.h"

namespace rageam::integration
{
	// TODO: Breaks if used inside a vehicle

	class ICameraComponent : IUpdateComponent
	{
	protected:
		static inline ICameraComponent* sm_Active = nullptr;

	public:
		~ICameraComponent() override = default;

		virtual const rage::Vec3V& GetPosition() const = 0;
		virtual const rage::Vec3V& GetVelocity() = 0;
		virtual const rage::Mat44V& GetMatrix() const = 0;
		virtual void SetMatrix(const rage::Mat44V& mtx) = 0;
		virtual void SetPosition(const rage::Vec3V& position) = 0;
		virtual void LookAt(const rage::Vec3V& point) = 0;

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

		static void EnableCameraAsync(int camera);
		static void DisableCameraAsync(int camera);
		static void DestroyCameraAsync(int camera);
		static void IncreaseShadowRenderDistance();
		static void ResetShadowRenderDistance();
		static void DestroyBlipAsync(int blip);
		void CreateBlipAsync();

	public:
		CameraComponentBase() = default;
		~CameraComponentBase() override;

		void OnStart() override;
		void OnEarlyUpdate() override;

		void SetActive(bool active) override;

		const rage::Vec3V& GetPosition() const override;
		const rage::Vec3V& GetVelocity() override;
		const rage::Mat44V& GetMatrix() const override;
		void SetMatrix(const rage::Mat44V& mtx) override;
		void LookAt(const rage::Vec3V& point) override;
	};

	class FreeCamera : public CameraComponentBase
	{
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
