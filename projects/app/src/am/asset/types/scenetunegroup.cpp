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
	u32 newTuneCount = GetItemCount(scene);
	List<SceneTunePtr> newTunes;
	newTunes.Resize(newTuneCount);

	// Set existing tunes
	for (SceneTunePtr& tune : Items)
	{
		int indexInScene = IndexOf(scene, tune->Name);
		if (indexInScene == -1)
		{
			AM_DEBUGF("SceneTuneGroup (%s) :: Refresh -> Removing tune for '%s'", 
				GetName(), tune->Name.GetCStr());
			continue;
		}

		// Map to the scene index
		newTunes[indexInScene] = std::move(tune);
	}

	// Add new ones
	for (u32 i = 0; i < newTuneCount; i++)
	{
		if (!newTunes[i]) // Resize initializes it to NULL
		{
			SceneTunePtr tune = CreateDefaultTune(scene, i);
			tune->Name = GetItemName(scene, i);
			AM_DEBUGF("SceneTuneGroup (%s) :: Refresh -> Created default tune for '%s'", 
				GetName(), tune->Name.GetCStr());

			newTunes[i] = std::move(tune);
		}
	}

	Items = std::move(newTunes);
}
