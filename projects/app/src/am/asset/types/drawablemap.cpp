#include "drawablemap.h"

#include "rage/physics/bounds/boundcomposite.h"
#include "rage/rmc/drawable.h"
#include "game/drawable.h"

rage::phBound* rageam::asset::DrawableAssetMap::GetBoundFromAbsoluteIndexRecurse(rage::phBound* bound, BoundAbsoluteIndex absoluteIndex, int& currentIndex) const
{
	if (absoluteIndex == currentIndex)
		return bound;

	// Bound is somewhere else
	if (bound->GetShapeType() != rage::PH_BOUND_COMPOSITE)
		return nullptr;
	
	// Skip composite...
	currentIndex++;

	// Unfold composite hierarchy
	rage::phBoundComposite* composite = bound->AsComposite();
	for (u16 i = 0; i < composite->GetNumBounds(); i++)
	{
		rage::phBound* compositeChild = composite->GetBound(i).Get();
		if (currentIndex == absoluteIndex)
			return compositeChild;

		// Nested composite, unfold further
		if (compositeChild->GetShapeType() == rage::PH_BOUND_COMPOSITE)
		{
			rage::phBound* matchedBound = GetBoundFromAbsoluteIndexRecurse(compositeChild, absoluteIndex, currentIndex);
			if (matchedBound)
				return matchedBound;
		}

		currentIndex++;
	}

	// Bound was not in this tree
	return nullptr;
}

rage::phBound* rageam::asset::DrawableAssetMap::GetBoundFromAbsoluteIndex(rage::phBound* bound, BoundAbsoluteIndex absoluteIndex) const
{
	int currentIndex = 0;
	rage::phBound* result = GetBoundFromAbsoluteIndexRecurse(bound, absoluteIndex, currentIndex);
	AM_ASSERT(result != nullptr, "DrawableAssetMap::GetBoundFromHandle() -> Absolute index %i is invalid!", absoluteIndex);
	return result;
}

rage::grmModel* rageam::asset::DrawableAssetMap::GetModelFromScene(rage::rmcDrawable* drawable, u16 sceneNodeIndex) const
{
	int lod = SceneNodeToLOD[sceneNodeIndex];
	if (lod == -1)
		return nullptr;
	u16 modelIndex = SceneNodeToModel[lod][sceneNodeIndex];
	if (modelIndex == u16(-1))
		return nullptr;
	return drawable->GetLodGroup().GetLod(lod)->GetModel(modelIndex);
}

rage::crBoneData* rageam::asset::DrawableAssetMap::GetBoneFromScene(const rage::rmcDrawable* drawable, u16 sceneNodeIndex) const
{
	u16 boneIndex = SceneNodeToBone[sceneNodeIndex];
	if (boneIndex == u16(-1))
		return nullptr;
	return drawable->GetSkeletonData()->GetBone(boneIndex);
}

rage::phBound* rageam::asset::DrawableAssetMap::GetBoundFromScene(const gtaDrawable* drawable, u16 sceneNodeIndex, u16 arrayIndex) const
{
	// To keep compatablity with old code, although this seems weird
	if (!SceneNodeToBound[sceneNodeIndex].Any())
		return nullptr;

	BoundAbsoluteIndex absoluteIndex = SceneNodeToBound[sceneNodeIndex][arrayIndex];
	if (absoluteIndex == -1)
		return nullptr;

	return GetBoundFromAbsoluteIndex(drawable->GetBound().Get(), absoluteIndex);
}

u16 rageam::asset::DrawableAssetMap::GetBoundCountFromScene(const gtaDrawable* drawable, u16 sceneNodeIndex) const
{
	return SceneNodeToBound[sceneNodeIndex].GetSize();
}

CLightAttr* rageam::asset::DrawableAssetMap::GetLightFromScene(gtaDrawable* drawable, u16 sceneNodeIndex) const
{
	u16 lightAttrIndex = SceneNodeToLightAttr[sceneNodeIndex];
	if (lightAttrIndex == u16(-1))
		return nullptr;
	return drawable->GetLight(lightAttrIndex);
}

void rageam::asset::DrawableAssetMap::Reset(int nodeCount, int materialCount)
{
	SceneNodeToLOD.Resize(nodeCount);
	SceneNodeToBound.Resize(nodeCount);
	for (s32 lod = 0; lod < MAX_LOD; lod++)
		SceneNodeToModel[lod].Resize(nodeCount);
	SceneNodeToBone.Resize(nodeCount);
	SceneNodeToLightAttr.Resize(nodeCount);
	SceneNodeToBoundType.Resize(nodeCount);
	SceneNodeToBoundIsInBVH.Resize(nodeCount);
	SceneMaterialToShader.Resize(materialCount);
	SceneMaterialToBounds.Resize(materialCount);
	for (s32& i : SceneNodeToLOD) i = -1;
	for (List<BoundAbsoluteIndex>& boundList : SceneNodeToBound)
		boundList = {};
	// Default everything to -1
	for (s32 lod = 0; lod < MAX_LOD; lod++)
		for (u16& i : SceneNodeToModel[lod]) 
			i = -1;
	for (u16& i : SceneNodeToBone)			i = u16(-1);
	for (u16& i : SceneNodeToLightAttr)		i = u16(-1);
	for (u16& i : SceneMaterialToShader)	i = u16(-1);
	for (List<BoundMaterialHandle>& i : SceneMaterialToBounds)	i = {};
}

void rageam::asset::DrawableAssetMap::AddNewNode()
{
	SceneNodeToLOD.Add(-1);
	SceneNodeToBound.Construct();
	for (int lod = 0; lod < MAX_LOD; lod++)
		SceneNodeToModel[lod].Add(u16(-1));
	SceneNodeToBone.Add(u16(-1));
	SceneNodeToLightAttr.Add(u16(-1));
	SceneNodeToBoundType.Add(graphics::PrimitiveInvalid);
	SceneNodeToBoundIsInBVH.Add(false);
}

void rageam::asset::DrawableAssetMap::AddNewMaterial()
{
	SceneMaterialToBounds.Construct();
	SceneMaterialToShader.Add(u16(-1));
}

void rageam::asset::DrawableAssetMap::DebugPrint() const
{
	Logger::GetInstance()->GetOptions().Set(LOG_OPTION_NO_PREFIX, true);

	AM_TRACEF("== Drawable Map ==");

	AM_TRACEF("Scene Node To: {");

	// NODE TO LOD
	AM_TRACEF("\tLOD");
	for (s32 i : SceneNodeToLOD) AM_TRACEF("\t\t%i", i);
	// NODE TO BOUND
	AM_TRACEF("\tBound");
	for (s32 i = 0; i < SceneNodeToBound.GetSize(); i++)
	{
		const List<BoundAbsoluteIndex>& nodeBoundIndices = SceneNodeToBound[i];
		AM_TRACEF("\t\t%i", i);
		for (BoundAbsoluteIndex boundIndex : nodeBoundIndices)
			AM_TRACEF("\t\t\t%i", boundIndex);
	}
	// NODE TO MODEL
	AM_TRACEF("\tModel");
	for (s32 lod = 0; lod < MAX_LOD; lod++)
	{
		AM_TRACEF("\t\tLOD%i", lod);
		for (u16 model : SceneNodeToModel[lod])
			AM_TRACEF("\t\t\t%i", (s16) model);
	}
	// NODE TO BONE
	AM_TRACEF("\tBone");
	for (u16 i : SceneNodeToBone) AM_TRACEF("\t\t%i", (s16) i);
	// NODE TO LIGHT
	AM_TRACEF("\tLight");
	for (u16 i : SceneNodeToLightAttr) AM_TRACEF("\t\t%i", (s16) i);
	// NODE TO BOUND TYPE
	AM_TRACEF("\tBoundType");
	for (int i : SceneNodeToBoundType) AM_TRACEF("\t\t%i", i);

	AM_TRACEF("");

	// MATERIAL TO SHADER
	AM_TRACEF("\tMaterial To Shader:");
	for (u16 i : SceneMaterialToShader) AM_TRACEF("\t\t%i", (s16) i);
	// MATERIAL TO BOUND(S)
	AM_TRACEF("\tMaterial To Bound:");
	for (List<BoundMaterialHandle>& boundMaterialHandles : SceneMaterialToBounds)
		for (BoundMaterialHandle boundMaterialHandle : boundMaterialHandles)
			AM_TRACEF("\t\t%i %i", boundMaterialHandle.AbsoluteBoundIndex, boundMaterialHandle.MaterialIndex);
	// MODEL TO NODE
	AM_TRACEF("\tModel To Node:");
	for (s32 lod = 0; lod < MAX_LOD; lod++)
	{
		AM_TRACEF("\t\tLOD%i", lod);
		for (u16 node : ModelToSceneNode[lod])
			AM_TRACEF("\t\t\t%i", node);
	}
	// Other
	AM_TRACEF("\tBound To Node:");
	for (u16 i : BoundToSceneNode) AM_TRACEF("\t\t%i", (s16) i);
	AM_TRACEF("\tBone To Node:");
	for (u16 i : BoneToSceneNode) AM_TRACEF("\t\t%i", (s16) i);
	AM_TRACEF("\tLight To Node:");
	for (u16 i : LightAttrToSceneNode) AM_TRACEF("\t\t%i", (s16) i);

	AM_TRACEF("}");
	AM_TRACEF("== Drawable Map ==");

	Logger::GetInstance()->GetOptions().Set(LOG_OPTION_NO_PREFIX, false);
}
