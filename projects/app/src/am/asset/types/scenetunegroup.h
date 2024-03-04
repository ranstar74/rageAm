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
	/**
	 * \brief SceneTune holds extra properties for scene component, for example
	 * user settings for such things as scene meshes, lights, geometries.
	 */
	struct SceneTune : IXml
	{
		string Name;

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
		List<SceneTunePtr> Items;

		SceneTuneGroupBase() = default;
		SceneTuneGroupBase(const SceneTuneGroupBase& other)
		{
			Items.Reserve(other.Items.GetSize());
			for (const SceneTunePtr& item : other.Items)
			{
				Items.Emplace(item->Clone());
			}
		}

		// NOTE: This also creates node container for the list! Using GetName() function
		void Serialize(XmlHandle& node) const override;
		void Deserialize(const XmlHandle& node) override;

		virtual int IndexOf(const graphics::Scene* scene, ConstString itemName) const = 0;

		virtual SceneTunePtr CreateDefaultTune(graphics::Scene* scene, u16 itemIndex) const = 0;
		virtual SceneTunePtr CreateTune() const = 0;

		virtual ConstString  GetItemName(graphics::Scene* scene, u16 itemIndex) const = 0;
		virtual u16			 GetItemCount(graphics::Scene* scene) const = 0;

		// Group name as it will appear in the config (for e.g. 'Materials')
		virtual ConstString GetName() const = 0;

		// Deletes all unused tunes (that are not in the scene anymore) and adds tunes for new ones
		// Makes sure that all the elements are mapped in the same order as in scene
		void Refresh(graphics::Scene* scene);
	};

	template<typename TSceneTune>
	struct SceneTuneGroup : SceneTuneGroupBase
	{
		SceneTuneGroup() = default;
		SceneTuneGroup(const SceneTuneGroup& other) : SceneTuneGroupBase(other) {}

		u16 GetCount() const { return Items.GetSize(); }
		amPtr<TSceneTune> Get(u16 index) const { return std::reinterpret_pointer_cast<TSceneTune>(Items[index]); }

		amPtr<TSceneTune>* begin() const { return (amPtr<TSceneTune>*)Items.begin(); }
		amPtr<TSceneTune>* end() const { return (amPtr<TSceneTune>*)Items.end(); }
	};
}
