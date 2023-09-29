 #pragma once

#include "rage/atl/array.h"
#include "helpers/macro.h"

namespace rage
{
	/**
	 * \brief Contains functions for dynamic allocation / placement of resource type with several delivered types.
	 * \n Type is resolved by enum element index.
	 * \tparam TRet Base resource type.
	 * \tparam TEnum Resource type enumeration. (It's index used in type array).
	 */
	template<typename TRet, typename TEnum>
	struct pgTypeInfo
	{
		typedef TRet* (*TAllocate);
		typedef TRet* (*TPlace)(const datResource&, TRet*);

		TEnum Type;
		const char* Name;
		TAllocate Allocate;
		TPlace Place;

		/**
		 * \brief Gets array index for given type.
		 */
		u16 GetIndex() const { return static_cast<u16>(Type); }
	};

	/**
	 * \brief Contains registered type information for dynamic resource placement.
	 * \n For example rage::crClip have 3 delivered types:
	 * \n - crClipAnimation
	 * \n - crClipAnimations
	 * \n - crClipAnimationExpression
	 * \n To dynamically resolve correct type on resource placement, type is stored in each struct type.
	 * During placement, pgTypeInfos::GetPlaceFn is used to resolve placement function.
	 * \tparam TRet Base resource type.
	 * \tparam TEnum Resource type enumeration. (It's index used in type array).
	 */
	template<typename TRet, typename TEnum>
	class pgTypeInfos
	{
		typedef pgTypeInfo<TRet, TEnum> TInfo;

		atArray<pgTypeInfo<TRet, TEnum>> m_Items;

		void Register(const TInfo& info) { m_Items[info.GetIndex()] = info; }
	public:
		pgTypeInfos() : m_Items(0) {}

		/**
		 * \brief Allocates memory for given number of type infos.
		 */
		void Init(u16 numTypes)
		{
			m_Items.Reserve(numTypes);
			for (u16 i = 0; i < numTypes; i++)
				m_Items.Add({}); // Add placeholders
		}

		/**
		 * \brief Registers new data type info.
		 * \param type Type enum item.
		 * \param name Debug display name.
		 * \param allocate Pointer to default constructor function.
		 * \param place Pointer to resource placement function.
		 * \tparam TDRet Derived class type. For e.g. for crClip it may be crClipAnimation;
		 */
		template<typename TDRet>
		void Register(
			TEnum type,
			const char* name,
			TDRet* (allocate)(),
			TDRet* (place)(const datResource&, TDRet*))
		{
			TInfo info
			{
				type,
				name,
				// We have to cast derived class back in to base class
				// for function prototype to satisfy compiler
				reinterpret_cast<TRet * (*)>(allocate),
				reinterpret_cast<TRet * (*)(const datResource&, TRet*)>(place)
			};
			Register(info);
		}

		/**
		 * \brief Retrieves type info for type.
		 */
		const TInfo& GetInfo(TEnum type)
		{
			u16 index = static_cast<u16>(type);
			return m_Items[index];
		}

		const char* GetName(TEnum type) { return GetInfo(type).Name; }
		typename TInfo::TAllocate GetAllocateFn(TEnum type) { return GetInfo(type).Allocate; }
		typename TInfo::TPlace GetPlaceFn(TEnum type) { return GetInfo(type).Place; }
	};

#define IMPLEMENT_TYPE_RESOLVE(name, infos) \
	static name* ResolveType(const rage::datResource& rsc, name* that) \
	{ \
		rsc.Fixup(that); \
		infos.GetPlaceFn(that->m_Type)(rsc, that); \
		return that; \
	} \
	MACRO_END
}
