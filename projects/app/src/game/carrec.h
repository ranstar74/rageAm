#pragma once

#include "math/vector3.h"
#include "paging/datResource.h"
#include "paging/pgBase.h"
#include "paging/place.h"
#include "paging/template/array.h"

static constexpr u32 MAX_CARREC_NAME = 72;

inline void MakeVehicleRecordingName(String buffer, u32 fileNumber, ConstString recordingName)
{
	sprintf_s(buffer, MAX_CARREC_NAME, "%s%03d", recordingName, fileNumber);
}

class CVehicleRecording : public rage::pgBase
{
	/**
	 * \brief Vehicle state in a point of time.
	 */
	struct Node
	{
		u32 Time;

		char gap0[6];

		s8 RightX;
		s8 RightY;
		s8 RightZ;
		s8 ForwardX;
		s8 ForwardY;
		s8 ForwardZ;

		//s32 dword10;


		float PosX;
		float PosY;
		float PosZ;
	};

	rage::pgArray<Node> m_Nodes;
public:
	CVehicleRecording() = default;
	CVehicleRecording(const rage::datResource& rsc) : m_Nodes(rsc) {}

	struct Transform
	{
		rage::Vec3V Right;
		rage::Vec3V Forward;
		rage::Vec3V Up;
		rage::Vec3V Pos;

		Transform(Node node)
		{
			Right.X = (float)node.RightX;
			Right.Y = (float)node.RightY;
			Right.Z = (float)node.RightZ;
			Forward.X = (float)node.ForwardX;
			Forward.Y = (float)node.ForwardY;
			Forward.Z = (float)node.ForwardZ;

			// Right & Forward are stored in 8 bits, remap them back to actual range
			Right /= 127.0f;
			Forward /= 127.0f;

			// Value quantization comes with precision lost so we have to recompute
			// orientation vectors to make sure there's perfect straight angle
			Up = Forward.Cross(Right);
			Up.Normalize();
			Right = Up.Cross(Forward);
			Right.Normalize();
			Forward = Up.Cross(Right);

			Pos = { node.PosX, node.PosY, node.PosZ };
		}
	};

	u32 GetNodeCount() const { return m_Nodes.GetSize(); }
	Transform GetTransform(u32 index) const { return Node(m_Nodes[index]); }

	IMPLEMENT_PLACE_INLINE(CVehicleRecording);
};
