#include "scenetunegroup.h"

#include "am/xml/iterator.h"

void rageam::asset::SceneTuneGroupBase::Serialize(XmlHandle& node) const
{
	XmlHandle xTunes = node.AddChild(GetName());
	for (const SceneTunePtr& tune : Items)
	{
		XmlHandle xTune = xTunes.AddChild("Item");
		tune->Serialize(xTune);
	}
}

void rageam::asset::SceneTuneGroupBase::Deserialize(const XmlHandle& node)
{
	XmlHandle xTunes = node.GetChild(GetName());
	for (const XmlHandle& xTune : XmlIterator(xTunes, "Item"))
	{
		SceneTunePtr tune = CreateTune();
		tune->Deserialize(xTune);
		Items.Emplace(std::move(tune));
	}
}

void rageam::asset::SceneTuneGroupBase::Refresh(graphics::Scene* scene)
{
	// Mark tunes that are not in the scene anymore as removed
	for (u16 i = 0; i < Items.GetSize(); i++)
	{
		SceneTunePtr& tune = Items[i];
		tune->IsRemoved = !ExistsInScene(scene, tune);
	}

	// We need this for ContainsTune, refresh is done after deserializing
	// so map is not built yet
	RebuildNameMap();

	// Add tune for new items
	for (u16 i = 0; i < GetSceneItemCount(scene); i++)
	{
		ConstString itemName = GetItemName(scene, i);
		if (!ContainsTune(itemName))
		{
			SceneTunePtr tune = CreateDefaultTune(scene, i);
			tune->Name = itemName;
			Items.Emplace(std::move(tune));
		}
	}

	// Now we have to sort all items to match scene indices
	SmallList<SceneTunePtr> sortedItems;
	sortedItems.Resize(Items.GetSize());
	for (u16 i = 0; i < Items.GetSize(); i++)
	{
		int sceneIndex = IndexOf(scene, Items[i]->Name);
		sortedItems[sceneIndex] = std::move(Items[i]);
	}
	Items = std::move(sortedItems);

	RebuildNameMap();
}

bool rageam::asset::SceneTuneGroupBase::ContainsTune(ConstString name) const
{
	return NameToItem.ContainsAt(Hash(name));
}

void rageam::asset::SceneTuneGroupBase::RebuildNameMap()
{
	NameToItem.InitAndAllocate(Items.GetSize());
	for (u16 i = 0; i < Items.GetSize(); i++)
	{
		NameToItem.InsertAt(Hash(Items[i]->Name), i);
	}
}
