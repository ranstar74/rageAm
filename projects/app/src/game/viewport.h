#pragma once

#include "rage/math/mtxv.h"

class CViewport
{
public:
	static const rage::Mat44V& GetViewMatrix();
	static const rage::Mat44V& GetViewProjectionMatrix();
	static const rage::Mat44V& GetProjectionMatrix();
};
