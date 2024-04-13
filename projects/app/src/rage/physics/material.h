//
// File: material.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/graphics/color.h"
#include "rage/atl/conststring.h"
#include "rage/dat/base.h"

namespace rage
{
	class fiAsciiTokenizer;

	using phMaterialColor = rageam::graphics::ColorU32;

	struct phMaterial : datBase
	{
		int         Type;
		ConstString Name;
		float       Friction;
		float       Elasticity;
		u8          Pad[12];

		virtual bool LoadData(fiAsciiTokenizer& tokenizer) { return false; }
		virtual void SaveData(fiAsciiTokenizer& tokenizer) { }
		virtual phMaterialColor GetDebugColor() { return rageam::graphics::COLOR_RED; }
	};

	class phMaterialMgr : public datBase
	{
	public:
		using Id = u64;

		static constexpr Id DEFAULT_MATERIAL_ID = 0;
		static constexpr Id MATERIAL_NOT_FOUND = Id(-1);

		struct phMaterialPair
		{
			Id    MaterialIndexA;
			Id    MaterialIndexB;
			float CombinedFriction;
			float CombinedElasticity;
		};

		phMaterialMgr() = delete;

		virtual void Load(int reservedSlots = 0) = 0;
		virtual void Destroy() = 0;

		virtual phMaterial*     FindMaterial(ConstString name) const = 0;
		virtual Id              FindMaterialId(ConstString name) const = 0;
		virtual void            GetMaterialName(Id id, char* buffer, int bufferSize) const = 0;
		virtual void            GetMaterialId(const phMaterial& material) const = 0;
		virtual phMaterial*     GetDefaultMaterial() const = 0;
		virtual phMaterialColor GetDebugColor(const phMaterial& material) = 0; // Hardcoded red
		virtual int             GetFlags(Id id) = 0; // MTLFLAG_

		u32         GetNumMaterials() const { return m_NumMaterials; }
		phMaterial* GetMaterialByIndex(u32 index) const;
		phMaterial* GetMaterialById(Id id) const;

		static phMaterialMgr* GetInstance();

	private:
		atConstString           m_AssetFolder;
		u64                     m_MaterialIndexMask; // 0xFF -> gtaMaterialId::GetId
		u32                     m_NumMaterials;
		u32                     m_MaterialsSize;
		char*                   m_Materials;
		atArray<phMaterialPair> m_MaterialOverridePairs;
	};
}
