#pragma once

#ifdef AM_INTEGRATED

#include "am/asset/factory.h"
#include "am/asset/types/drawable.h"
#include "am/file/watcher.h"
#include "am/integration/gameentity.h"
#include "game/drawable.h"
#include "integration/modelscene.h"

#endif

#include "am/types.h"
#include "am/file/watcher.h"
#include "am/ui/app.h"
#include "am/ui/extensions.h"
#include "game/viewport.h"

namespace rageam::ui
{
	class TestbedApp : public App
	{
	public:
		TestbedApp()
		{

		}

		// Perspective projection matrix with inverse depth
		static Mat44V InversePerspective(float fov, float aspect, float nearZ = 0.1f, float farZ = 7333.0f)
		{
			std::swap(nearZ, farZ);

			fov *= 0.5f;

			float fovCos = cosf(fov);
			float fovSin = sinf(fov);
			float height = fovCos / fovSin;
			float range = farZ / (farZ - nearZ);

			Mat44V m;
			m.R[0].SetX(height / aspect);
			m.R[1].SetY(height);
			m.R[2].SetZ(range);
			m.R[2].SetW(-1.0f);
			m.R[3].SetZ(farZ);
			m.R[3].SetW(0.0f);
			return m;
		}

		DirectX::XMMATRIX XM_CALLCONV XMMatrixPerspectiveFovLH(float FovAngleY, float AspectRatio, float NearZ, float FarZ)
		{
			float    SinFov;
			float    CosFov;
			DirectX::XMScalarSinCos(&SinFov, &CosFov, 0.5f * FovAngleY);

			float fRange = FarZ / (FarZ - NearZ);
			// Note: This is recorded on the stack
			float Height = CosFov / SinFov;
			DirectX::XMVECTOR rMem = {
				Height / AspectRatio,
				Height,
				-fRange,
				-fRange * NearZ
			};
			// Copy from memory to SSE register
			DirectX::XMVECTOR vValues = rMem;
			DirectX::XMVECTOR vTemp = _mm_setzero_ps();
			// Copy x only
			vTemp = _mm_move_ss(vTemp, vValues);
			// CosFov / SinFov,0,0,0
			DirectX::XMMATRIX M;
			M.r[0] = vTemp;
			// 0,Height / AspectRatio,0,0
			vTemp = vValues;
			vTemp = _mm_and_ps(vTemp, DirectX::g_XMMaskY);
			M.r[1] = vTemp;
			// x=fRange,y=-fRange * NearZ,0,1.0f
			vTemp = _mm_setzero_ps();
			vValues = _mm_shuffle_ps(vValues, { 0, 0, 0, -1 }, _MM_SHUFFLE(3, 2, 3, 2));
			// 0,0,fRange,1.0f
			vTemp = _mm_shuffle_ps(vTemp, vValues, _MM_SHUFFLE(3, 0, 0, 0));
			M.r[2] = vTemp;
			// 0,0,-fRange * NearZ,0.0f
			vTemp = _mm_shuffle_ps(vTemp, vValues, _MM_SHUFFLE(2, 1, 0, 0));
			M.r[3] = vTemp;
			return M;
		}

		void OnRender() override
		{
			return;
			ImGui::Begin("rageAm Testbed");

			if (ImGui::Button("Ray"))
			{
				Mat44V world;
				world.R[0] = Vec4V(1.00, 0.00, 0.00, 0.00);
				world.R[1] = Vec4V(0.00, 1.00, 0.00, 0.00);
				world.R[2] = Vec4V(0.00, 0.00, 1.00, 0.00);
				world.R[3] = Vec4V(-676.00, 167.00, 73.55, 1.00);

				/*gWorld{ 1.00, 0.00, 0.00, 0.00 }
				{0.00, 1.00, 0.00, 0.00}
				{0.00, 0.00, 1.00, 0.00}
				{-676.00, 167.00, 73.55, 1.00} 0 float4x4(row_major)*/

				/*		gWorldViewProj.row0 0.64521, 0.47884, -0.00001, 0.8149        float4
							gWorldViewProj.row1 - 1.01923, 0.30312, -8.42480E-06, 0.51586    float4
							gWorldViewProj.row2 0.00, 2.06827, 4.31592E-06, -0.26427      float4
							gWorldViewProj.row3 - 0.69055, 0.64729, 0.09994, 3.76283       float4*/

				Mat44V worldViewProj;
				worldViewProj.R[0] = Vec4V(0.64521, 0.47884, -0.00001, 0.8149);
				worldViewProj.R[1] = Vec4V(-1.01923, 0.30312, -8.42480E-06, 0.51586);
				worldViewProj.R[2] = Vec4V(0.00, 2.06827, 4.31592E-06, -0.26427);
				worldViewProj.R[3] = Vec4V(-0.69055, 0.64729, 0.09994, 3.76283);

				Mat44V worldView;
				worldView.R[0] = Vec4V(0.53487, 0.22329, -0.8149, 0.00);
				worldView.R[1] = Vec4V(-0.84493, 0.14135, -0.51586, 0.00);
				worldView.R[2] = Vec4V(0.00, 0.96445, 0.26427, 0.00);
				worldView.R[3] = Vec4V(-0.57246, 0.30184, -3.76283, 1.00);

				//Mat44V m1 = worldView.Inverse() * worldViewProj;

				//worldViewProj *= worldView.Inverse();
				worldViewProj = worldView.Inverse() * worldViewProj;
				worldViewProj.R[0].SetY(0);
				worldViewProj.R[0].SetZ(0);
				worldViewProj.R[0].SetW(0);
				worldViewProj.R[1].SetX(0);
				worldViewProj.R[1].SetZ(0);
				worldViewProj.R[1].SetW(0);
				worldViewProj.R[2].SetX(0);
				worldViewProj.R[2].SetY(0);
				worldViewProj.R[3].SetX(0);
				worldViewProj.R[3].SetY(0);

				Mat44V worldViewProjInverse = worldViewProj.Inverse();
				Vec3V coordNear = Vec3V(0, 0, 0).Transform(worldViewProjInverse);
				Vec3V coordFar = Vec3V(0, 0, 1).Transform(worldViewProjInverse);
				Vec3V to = coordFar - coordNear;

				Vec4V test1 = Vec3V(-1, -1, 0).Transform4(worldViewProj);
				Vec3V test2 = Vec3V(-1, -1, 0).Transform(worldViewProj);

				Vec3V coordTest = test2.Transform(worldViewProjInverse);

				float fov = 2.0f * atanf(1.0f / worldViewProj.R[1][1]);
				//fov /= 3.14f;
				//fov *= 180.0f;

				float aspect = 1600.0f / 900.0f;
				Mat44V project = InversePerspective(fov, aspect, 0.1f, 7333.f);//XMMatrixPerspectiveFovLH(fov, aspect, 7333.0f, 0.1f);

				//project.R[3].SetW(2.38418579e-07f);
				Mat44V projInverse = worldViewProj.Inverse();
				Vec4V coordNear14 = Vec3V(0, 0, 0).Transform4(projInverse);
				Vec3V coordNear1 = Vec3V(0, 0, 0).Transform(projInverse);
				Vec3V coordFar1 = Vec3V(0, 0, 1).Transform(projInverse);
				Vec3V to1 = coordFar1 - coordNear1;

				AM_TRACEF("");
			}

			ImGui::End();
		}
	};
}
