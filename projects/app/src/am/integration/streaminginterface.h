#pragma once

#include "common/types.h"

namespace rageam::integration
{
	// TODO: Export Test

	class IStreamingModule
	{
	public:
		virtual ~IStreamingModule() = default;
		virtual bool IsModelExists(ConstString name) = 0;
		virtual bool Load(ConstString name) = 0;
		virtual bool LoadAsync(ConstString name) = 0;
		virtual void Unload() = 0;
		virtual bool IsLoaded() = 0;
	};

	class IStreamingInterface
	{
	public:
		virtual ~IStreamingInterface() = default;
		virtual IStreamingModule* GetModule(ConstString extension) = 0;
	};
}
