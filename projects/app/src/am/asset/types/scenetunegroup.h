//
// File: scenetunegroup.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/types.h"
#include "am/graphics/scene.h"
#include "am/xml/serialize.h"

namespace rageam::asset
{
	// Scene tune groups were introduced as solution for problem of synchronizing
	// scene changes with config, for example what do we do if user removes material? or light?
	// SceneTuneGroup::Refresh() keeps track of scene changes and handles them accordingly

	/**
	 * \brief Additional properties that user can attach to scene element.
	 */
	struct SceneTune : IXml
	{
		string	Name;
		// Scene element this tune was attached to was removed, and now this tune is unused
		// we mark it instead of removing to prevent indexing issues in SceneTuneGroup::Items array
		// Removed tunes are simply not written to the config file on saving, additionally
		// this gives us more flexibility for undo system
		bool	IsRemoved = false;

		void Serialize(XmlHandle& node) const override { XML_SET_ATTR(node, Name); }
		void Deserialize(const XmlHandle& node) override { XML_GET_ATTR(node, Name); }

		virtual amPtr<SceneTune> Clone() const = 0;
	};
	using SceneTunePtr = amPtr<SceneTune>;

	/**
	 * \brief Patch-like configuration for scene elements, for example lights or materials.
	 */
	struct SceneTuneGroupBase : IXml
	{
		SmallList<SceneTunePtr> Items;
		HashSet<u16>			NameToItem;

		SceneTuneGroupBase() = default;
		SceneTuneGroupBase(const SceneTuneGroupBase& other) : NameToItem(other.NameToItem)
		{
			Items.Reserve(other.Items.GetSize());
			for (const SceneTunePtr& item : other.Items)
			{
				Items.Emplace(item->Clone());
			}
		}

		void Serialize(XmlHandle& node) const override;
		void Deserialize(const XmlHandle& node) override;

		virtual bool ExistsInScene(graphics::Scene* scene, const SceneTunePtr& tune) const = 0;
		virtual SceneTunePtr CreateDefaultTune(graphics::Scene* scene, u16 itemIndex) const = 0;
		virtual SceneTunePtr CreateTune() const = 0;
		virtual ConstString GetItemName(graphics::Scene* scene, u16 itemIndex) const = 0;
		virtual u16 GetSceneItemCount(graphics::Scene* scene)const = 0;
		// Name as it will appear in the config
		virtual ConstString GetName() const = 0;
		// Deletes all unused tunes (that are not in the scene anymore) and adds tunes for new ones
		void Refresh(graphics::Scene* scene);
		bool ContainsTune(ConstString name) const;

		void RebuildNameMap();
	};

	template<typename TSceneTune>
	struct SceneTuneGroup : SceneTuneGroupBase
	{
		SceneTuneGroup() = default;
		SceneTuneGroup(const SceneTuneGroup& other) : SceneTuneGroupBase(other) {}

		u16 GetCount() const { return Items.GetSize(); }
		const amPtr<TSceneTune>& Get(u16 index) const { return std::reinterpret_pointer_cast<TSceneTune>(Items[index]); }

		amPtr<TSceneTune>* begin() const { return (amPtr<TSceneTune>*)Items.begin(); }
		amPtr<TSceneTune>* end() const { return (amPtr<TSceneTune>*)Items.end(); }
	};
}
