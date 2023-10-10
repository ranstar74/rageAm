#include "modelscene.h"

#include "am/ui/font_icons/icons_awesome.h"

#ifdef AM_INTEGRATED

#include "ImGuizmo.h"
#include "am/graphics/shapetest.h"

#include "am/ui/font_icons/icons_am.h"
#include "am/ui/styled/slwidgets.h"
#include "rage/paging/builder/builder.h"

#include <shlobj_core.h>

#include "am/task/worker.h"
#include "rage/math/math.h"

#include "am/ui/im3d.h"

void rageam::ModelScene::CreateEntity(const rage::Vec3V& coors)
{
	DeleteEntity();

	m_EntityPos = coors;
	m_EntityHandle = SHV::OBJECT::CREATE_OBJECT_NO_OFFSET(
		RAGEAM_HASH, m_EntityPos.X(), m_EntityPos.Y(), m_EntityPos.Z(), FALSE, TRUE, FALSE);

	static auto getEntity = gmAddress::Scan("E8 ?? ?? ?? ?? 90 EB 4B").GetCall().ToFunc<u64(u32)>();
	m_Entity = getEntity(m_EntityHandle);
}

void rageam::ModelScene::DeleteEntity()
{
	if (m_EntityHandle == 0)
		return;

	SHV::ENTITY::SET_ENTITY_AS_MISSION_ENTITY(m_EntityHandle, FALSE, TRUE);
	SHV::OBJECT::DELETE_OBJECT(&m_EntityHandle);
	m_Entity = 0;
}

void rageam::ModelScene::LoadAndCompileDrawableAsync(ConstWString path)
{
	DeleteDrawable();

	file::WPath wPath = path;

	m_FileWatcher.SetEnabled(false);
	m_LoadTask = BackgroundWorker::Run([&, wPath]
		{
			amPtr<asset::DrawableAsset> asset = asset::AssetFactory::LoadFromPath<asset::DrawableAsset>(wPath);
			if (!asset)
			{
				AM_ERRF(L"ModelScene::LoadAndCompileDrawable() -> Failed to load drawable from path %ls", wPath.GetCStr());
				return false;
			}

			gtaDrawable* drawable = new gtaDrawable();
			if (!asset->CompileToGame(drawable))
			{
				AM_ERRF(L"ModelScene::LoadAndCompileDrawable() -> Failed to compile drawable from path %ls", wPath.GetCStr());
				return false;
			}

			std::unique_lock lock(m_Mutex);
			m_FileWatcher.SetEntry(String::ToUtf8Temp(asset->GetDrawableModelPath()));
			m_Drawable = amUniquePtr<gtaDrawable>(drawable);
			m_DrawableAsset = std::move(asset);
			m_FileWatcher.SetEnabled(true);

			return true;
		});
}

void rageam::ModelScene::RegisterArchetype()
{
	if (m_Archetype)
		UnregisterArchetype();

	static gmAddress initArchetypeFromDef_Addr = gmAddress::Scan("48 89 5C 24 08 57 48 83 EC 20 49 8B F8 48 8B D9 E8 ?? ?? ?? ?? 44");
	static void(__fastcall * initArchetypeFromDef)(CBaseModelInfo*, rage::strLocalIndex, rage::fwArchetypeDef*, bool) =
		initArchetypeFromDef_Addr.To<decltype(initArchetypeFromDef)>();

	static gmAddress baseModelInfoCtor_Addr = gmAddress::Scan("65 48 8B 14 25 58 00 00 00 48 8D 05 ?? ?? ?? ?? 45");
	static void(__fastcall * baseModelInfoCtor)(CBaseModelInfo*) =
		baseModelInfoCtor_Addr.To<decltype(baseModelInfoCtor)>();

	CBaseModelInfo* modelInfo = (CBaseModelInfo*)rage_malloc(0xB0);
	baseModelInfoCtor(modelInfo);

	auto& drBb = m_Drawable->GetLodGroup().GetBoundingBox();
	auto& drBs = m_Drawable->GetLodGroup().GetBoundingSphere();

	CBaseArchetypeDef modelDef{};
	modelDef.m_Name = RAGEAM_HASH;
	modelDef.m_AssetType = rage::fwArchetypeDef::ASSET_TYPE_DRAWABLE;
	modelDef.m_BoundingBox = drBb;
	modelDef.m_BsCentre = drBs.GetCenter();
	modelDef.m_BsRadius = drBs.GetRadius().Get();
	modelDef.m_AssetName = RAGEAM_HASH;
	// TODO: This breaks on very large objects, for example plane covering whole map
	// modelDef.m_LodDist = 100;
	modelDef.m_LodDist = drBs.GetRadius().Get() + 100.0f; // Temporary solution for large models
	modelDef.m_PhysicsDictionary = RAGEAM_HASH;
	modelDef.m_Flags = rage::ADF_STATIC | rage::ADF_BONE_ANIMS;
	modelDef.m_TextureDictionary = RAGEAM_HASH;

	initArchetypeFromDef(modelInfo, 0, &modelDef, /*false*/ true);

	modelInfo->m_Flags |= 1; // Drawable loaded?

	// TODO: Must be done by init archetype...
	static gmAddress registerArchetype_Addr = gmAddress::Scan(
		"48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 20 48 83 3D ?? ?? ?? ?? ?? 8B FA");
	static void(__fastcall * registerArchetype)(CBaseModelInfo*, rage::strLocalIndex) =
		registerArchetype_Addr.To<decltype(registerArchetype)>();
	registerArchetype(modelInfo, 0);
	m_Archetype = modelInfo;
}

void rageam::ModelScene::UnregisterArchetype()
{
	if (!m_Archetype)
		return;

	AM_ASSERT(m_ArchetypeOld == nullptr, "ModelScene::UnregisterArchetype() -> Old archetype is not unloaded yet.");

	m_ArchetypeOld = m_Archetype;
	m_Archetype = nullptr;
}

void rageam::ModelScene::FinalizeOldArchetype()
{
	if (m_ArchetypeOld && m_ArchetypeOld->m_RefCount == 0)
	{
		static auto modelInfoDctor =
			gmAddress((*gmAddress::Scan("48 8D 05 ?? ?? ?? ?? 4C 89 49 58").GetRef(3).To<u64*>())).ToFunc<void(CBaseModelInfo*, bool)>();

		modelInfoDctor(m_ArchetypeOld, false);
		rage_free(m_ArchetypeOld);
		m_ArchetypeOld = nullptr;
	}
}

void rageam::ModelScene::RegisterDrawable()
{
	rage::strStreamingModule* drawableStore = hooks::Streaming::GetModule("ydr");
	if (m_DrawableSlot == rage::INVALID_STR_INDEX)
		drawableStore->AddSlot(m_DrawableSlot, RAGEAM_HASH);
	drawableStore->Set(m_DrawableSlot, m_Drawable.get());

	//rage::grcTextureDictionary* dict = m_Drawable->GetShaderGroup()->GetEmbedTextureDictionary();
	//if (dict)
	//{
	//	rage::strStreamingModule* txdStore = hooks::Streaming::GetModule("ytd");
	//	if (m_DictSlot == rage::INVALID_STR_INDEX)
	//	{
	//		txdStore->AddSlot(m_DictSlot, RAGEAM_HASH);
	//		dict->AddRef();
	//	}
	//	txdStore->Set(m_DictSlot, m_Drawable->GetShaderGroup()->GetEmbedTextureDictionary());
	//}
}

void rageam::ModelScene::UnregisterDrawable()
{
	rage::strStreamingModule* drawableStore = hooks::Streaming::GetModule("ydr");
	if (m_DrawableSlot != rage::INVALID_STR_INDEX)
	{
		drawableStore->Set(m_DrawableSlot, nullptr);
		drawableStore->RemoveSlot(m_DrawableSlot);
		m_DrawableSlot = rage::INVALID_STR_INDEX;
	}

	//rage::strStreamingModule* txdStore = hooks::Streaming::GetModule("ytd");
	//if (m_DictSlot != rage::INVALID_STR_INDEX)
	//{
	//	m_Drawable->GetShaderGroup()->GetEmbedTextureDictionary()->Release();
	//	txdStore->Set(m_DictSlot, nullptr);
	//	txdStore->RemoveSlot(m_DictSlot);
	//	m_DictSlot = rage::INVALID_STR_INDEX;
	//}
}

void rageam::ModelScene::RequestReload()
{
	m_HasModelRequest = true;
	if (m_IsLoaded)
		m_CleanUpRequested = true;
}

void rageam::ModelScene::DeleteDrawable()
{
	if (!m_Drawable)
		return;

	// Temporary hack until we have simple allocator back because os allocator can't detect
	// if block is valid or not so it cant dispatch delete to virtual allocator for builded resources
	if (m_Drawable->HasMap())
		GetAllocator(rage::ALLOC_TYPE_VIRTUAL)->Free(m_Drawable.release());
	else
		m_Drawable = nullptr;
}

bool rageam::ModelScene::OnAbort()
{
	// Wait for current loading task
	if (m_LoadTask)
		return false;

	m_CleanUpRequested = true;
	return m_IsLoaded == false;
}

void rageam::ModelScene::OnEarlyUpdate()
{
	std::unique_lock lock(m_Mutex);

	FinalizeOldArchetype();
}

void rageam::ModelScene::OnLateUpdate()
{
	std::unique_lock lock(m_Mutex);

	// Model file was changed, reload drawable
	if (m_FileWatcher.GetChangeOccuredAndReset())
	{
		RequestReload();
	}

	// We accept request only once previous entity was cleaned up
	if (m_HasModelRequest && !m_CleanUpRequested)
	{
		// Use drawable from request or load it from .idr
		if (m_DrawableRequest)
		{
			DeleteDrawable();
			m_Drawable = std::move(m_DrawableRequest);
		}
		else if (!m_LoadTask)
		{
			LoadAndCompileDrawableAsync(m_LoadRequest->Path);
		}

		// Use existing drawable (if we just want to respawn entity or if drawable
		// was set externally) or compile a new one from asset
		if (m_Drawable)
		{
			RegisterDrawable();
			RegisterArchetype();
			CreateEntity(m_EntityPos);
			m_IsLoaded = true;

			//if (m_Drawable->GetSkeletonData())
			//	m_Drawable->GetSkeletonData()->DebugPrint();

			if (LoadCallback) LoadCallback(m_DrawableAsset, m_Drawable.get());
			m_HasModelRequest = false;
			m_LoadTask = nullptr;
		}
	}

	if (m_LoadTask && m_LoadTask->IsFinished() && !m_LoadTask->IsSuccess())
	{
		// Failed to load...
		m_LoadTask = nullptr;
		m_HasModelRequest = false;
	}

	if (m_CleanUpRequested)
	{
		m_FileWatcher.SetEnabled(false);

		// Full entity unload takes more than one frame, we first delete
		// entity and on the next frame we clean up the rest
		if (m_EntityHandle != 0) // First tick
		{
			AM_DEBUGF("ModelScene -> Clean Up requested, removing entity...");
			DeleteEntity();
		}
		else // Second tick
		{
			AM_DEBUGF("ModelScene -> Finishing clean up");
			UnregisterArchetype();
			UnregisterDrawable();
			DeleteDrawable();
			m_IsLoaded = false;
			m_CleanUpRequested = false;
		}
	}
}

void rageam::ModelScene::SetupFor(ConstWString path, const rage::Vec3V& coors)
{
	SetEntityPos(coors);

	std::unique_lock lock(m_Mutex);
	m_LoadRequest = std::make_unique<LoadRequest>(path);
	RequestReload();
}

void rageam::ModelScene::CleanUp()
{
	std::unique_lock lock(m_Mutex);
	m_CleanUpRequested = true;
}

void rageam::ModelScene::SetEntityPos(const rage::Vec3V& pos)
{
	std::unique_lock lock(m_Mutex);
	m_EntityPos = pos;

	if (m_EntityHandle != 0)
	{
		scrInvoke([=]
			{
				SHV::ENTITY::SET_ENTITY_COORDS_NO_OFFSET(
					m_EntityHandle, pos.X(), pos.Y(), pos.Z(), FALSE, FALSE, TRUE);
			});
	}
}

void rageam::ModelScene::GetState(ModelSceneState& state)
{
	std::unique_lock lock(m_Mutex);

	if (m_Entity)
	{
		rage::Mat34V world = *(rage::Mat34V*)(m_Entity + 0x60);
		state.EntityWorld = world.To44();
	}
	state.EntityPtr = (pVoid)m_Entity;
	state.EntityHandle = m_EntityHandle;
	state.IsEntitySpawned = m_Entity != 0;
	state.IsLoading = m_LoadTask && !m_LoadTask->IsFinished();;
}

bool rageam::ModelSceneApp::SceneTreeNode(ConstString text, bool& selected, bool& toggled, SlGuiTreeNodeFlags flags)
{
	toggled = false;

	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiStorage* storage = window->DC.StateStorage;
	ImGuiContext* context = GImGui;
	ImGuiStyle& style = context->Style;
	ImGuiID id = window->GetID(text);

	bool& isOpen = *storage->GetBoolRef(id, flags & SlGuiTreeNodeFlags_DefaultOpen);

	ImVec2 cursor = window->DC.CursorPos;

	ImGui::ColumnsBeginBackground();
	// Stretch horizontally on whole window
	ImVec2 frameSize(window->ParentWorkRect.GetWidth(), ImGui::GetFrameHeight());
	ImVec2 frameMin(window->ParentWorkRect.Min.x, cursor.y);
	ImVec2 frameMax(frameMin.x + frameSize.x, frameMin.y + frameSize.y);
	//ImRect bb(frameMin, frameMax);

	// Based on whether we hover arrow / frame we select corresponding bounding box for button
	constexpr float arrowSizeX = 18.0f; // Eyeballed
	ImVec2 arrowMin(cursor.x + style.FramePadding.x, frameMin.y);
	ImVec2 arrowMax(arrowMin.x + arrowSizeX, frameMax.y);
	ImRect arrowBb(arrowMin, arrowMax);

	// Item bounding it set to only arrow to allow us to add more items in node later using ImGui::SameLine
	ImRect itemBb(arrowMin, arrowMax);
	ImRect bb(frameMin, frameMax);

	ImGui::ItemSize(itemBb.GetSize(), 0.0f);
	bool added = ImGui::ItemAdd(itemBb, id);
	ImGui::ColumnsEndBackground();
	if (!added)
	{
		if (isOpen)
			ImGui::TreePushOverrideID(id);

		return isOpen;
	}

	ImVec2 textSize = ImGui::CalcTextSize(text);
	float centerTextY = IM_GET_CENTER(frameMin.y, frameSize.y, textSize.y);

	ImVec2 arrowPos(arrowMin.x, centerTextY);

	bool hoversArrow = ImGui::IsMouseHoveringRect(arrowMin, arrowMax);

	ImGuiButtonFlags buttonFlags = ImGuiButtonFlags_MouseButtonLeft;
	if (flags & SlGuiTreeNodeFlags_RightClickSelect)
		buttonFlags |= ImGuiButtonFlags_MouseButtonRight;

	bool hovered, held;
	bool pressed = ImGui::ButtonBehavior(hoversArrow ? arrowBb : bb, id, &hovered, &held, buttonFlags);
	ImGui::SetItemAllowOverlap();

	if (flags & SlGuiTreeNodeFlags_DisplayAsHovered)
		hovered = true;

	if (!hoversArrow && pressed)
		selected = true;

	bool canOpen = !(flags & ImGuiIconTreeNodeFlags_NoChildren);
	if (canOpen)
	{
		// Toggle on simple arrow click
		if (pressed && hoversArrow)
			toggled = true;

		// Toggle on mouse double click
		if (flags & SlGuiTreeNodeFlags_RightClickSelect &&
			hovered &&
			context->IO.MouseClickedCount[ImGuiMouseButton_Left] == 2)
			toggled = true;

		// Arrow right opens node
		if (isOpen && context->NavId == id && context->NavMoveDir == ImGuiDir_Left)
		{
			toggled = true;
			ImGui::NavMoveRequestCancel();
		}

		// Arrow left closes node
		if (!isOpen && context->NavId == id && context->NavMoveDir == ImGuiDir_Right)
		{
			toggled = true;
			ImGui::NavMoveRequestCancel();
		}

		if (toggled)
		{
			isOpen = !isOpen;
			context->LastItemData.StatusFlags |= ImGuiItemStatusFlags_ToggledOpen;
		}
	}

	// Render
	{
		// Shrink it a little to create spacing between items
		// We do that after collision pass so you can't click in-between
		//bb.Expand(ImVec2(-1, -1));

		const SlGuiCol backgroundCol =
			selected ? SlGuiCol_GraphNodePressed : SlGuiCol_GraphNode;
		const SlGuiCol borderCol = selected ? SlGuiCol_GraphNodeBorderHighlight : SlGuiCol_None;

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);

		// Shadow, Background, Gloss, Border
		SlGui::RenderFrame(bb, SlGui::GetColorGradient(backgroundCol));
		SlGui::RenderBorder(bb, SlGui::GetColorGradient(borderCol));
		if (selected || hovered) SlGui::RenderGloss(bb, SlGuiCol_GlossBg);
		ImGui::PopStyleVar();
	}
	ImGui::RenderNavHighlight(bb, id, ImGuiNavHighlightFlags_TypeThin);

	// Arrow, we add slow fading in/out just like in windows explorer
	if (canOpen)
	{
		bool arrowVisible = context->HoveredWindow == window || ImGui::IsWindowFocused();

		float& alpha = *storage->GetFloatRef(id + 1);

		// Fade in fast, fade out slower...
		alpha += ImGui::GetIO().DeltaTime * (arrowVisible ? 4.0f : -2.0f);

		// Make max alpha level a little dimmer for sub-nodes
		float maxAlpha = window->DC.TreeDepth == 0 ? 0.8f : 0.6f;
		alpha = ImClamp(alpha, 0.0f, maxAlpha);

		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, hoversArrow ? maxAlpha : alpha);
		ImGui::RenderText(arrowPos, isOpen ? ICON_FA_ANGLE_DOWN : ICON_FA_ANGLE_RIGHT);
		ImGui::PopStyleVar(); // Alpha
	}

	const char* textDisplayEnd = ImGui::FindRenderedTextEnd(text);
	ImGui::SameLine();
	ImGui::AlignTextToFramePadding();
	ImGui::TextEx(text, textDisplayEnd);

	if (isOpen)
		ImGui::TreePushOverrideID(id);
	return isOpen;
}

rage::Vec3V rageam::ModelSceneApp::GetEntityScenePos() const
{
	return m_IsolatedSceneActive ? DEFAULT_ISOLATED_POS : m_ScenePosition;
}

void rageam::ModelSceneApp::UpdateDrawableStats()
{
	m_NumModels = 0;
	m_NumGeometries = 0;
	m_VertexCount = 0;
	m_TriCount = 0;
	m_LightCount = 0;

	m_LightCount = m_Drawable->m_Lights.GetSize();

	rage::rmcLodGroup& lodGroup = m_Drawable->GetLodGroup();
	const rage::spdAABB& boundingBox = lodGroup.GetBoundingBox();

	m_Dimensions = boundingBox.Max - boundingBox.Min;
	m_NumLods = lodGroup.GetLodCount();
	for (int i = 0; i < m_NumLods; i++)
	{
		rage::rmcLod* lod = lodGroup.GetLod(i);
		auto& lodModels = lod->GetModels();

		m_NumModels += lodModels.GetSize();
		for (auto& model : lodModels)
		{
			auto& modelGeoms = model->GetGeometries();

			m_NumGeometries += modelGeoms.GetSize();
			for (auto& geom : modelGeoms)
			{
				m_VertexCount += geom->GetVertexCount();
				m_TriCount += geom->GetPrimitiveCount();
			}
		}
	}
}

void rageam::ModelSceneApp::ResetCameraPosition()
{
	if (!m_Camera)
		return;

	rage::Vec3V camPos;
	rage::Vec3V targetPos;
	rage::Vec3V scenePos = m_IsolatedSceneActive ? DEFAULT_ISOLATED_POS : DEFAULT_POS;

	if (m_Drawable)
	{
		rage::rmcLodGroup& lodGroup = m_Drawable->GetLodGroup();
		auto& bb = lodGroup.GetBoundingBox();
		auto& bs = lodGroup.GetBoundingSphere();

		camPos = scenePos;
		// Shift camera away to fully see bounding sphere + add light padding
		camPos += rage::VEC_BACK * bs.GetRadius() * 1.5f;
		// Entities are spawned with bottom of bounding box aligned to specified coord
		targetPos = scenePos + rage::VEC_UP * bb.Height() * rage::S_HALF;
	}
	else
	{
		camPos = scenePos + rage::VEC_FORWARD;
		targetPos = scenePos;
	}

	m_Camera->SetPosition(camPos);
	m_Camera->LookAt(targetPos);
}

void rageam::ModelSceneApp::UpdateCamera()
{
	if (m_CameraEnabled)
	{
		if (m_UseOrbitCamera)
			m_Camera.Create<integration::OrbitCamera>();
		else
			m_Camera.Create<integration::FreeCamera>();

		ResetCameraPosition();
		m_Camera->SetActive(true);
	}
	else
	{
		m_Camera = nullptr;
	}
}

void rageam::ModelSceneApp::DrawSceneGraphRecurse(const graphics::SceneNode* sceneNode)
{
	if (!sceneNode)
		return;

	u16 nodeIndex = sceneNode->GetIndex();

	ImGui::TableNextRow();

	// Column: Node
	ImGui::TableNextColumn();
	ConstString treeNodeName = ImGui::FormatTemp("%s###NODE_%u", sceneNode->GetName(), sceneNode->GetIndex());
	SlGuiTreeNodeFlags treeNodeFlags =
		SlGuiTreeNodeFlags_DefaultOpen;
	if (sceneNode->GetFirstChild() == nullptr)
		treeNodeFlags |= SlGuiTreeNodeFlags_NoChildren;

	bool selected = m_SelectedNodeIndex == nodeIndex;
	bool toggled;
	bool opened = SceneTreeNode(treeNodeName, selected, toggled, treeNodeFlags);
	if (selected)
		m_SelectedNodeIndex = nodeIndex;

	// Attribute icons
	ImGui::SameLine(); ImGui::Dummy(ImVec2(2, 0)); // Spacing after text
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 4));
	auto grmModel = m_DrawableSceneMap.GetModelFromScene(m_Drawable, nodeIndex);
	auto crBoneData = m_DrawableSceneMap.GetBoneFromScene(m_Drawable, nodeIndex);
	auto lightAttr = m_DrawableSceneMap.GetLightFromScene(m_Drawable, nodeIndex);
	auto phBound = m_DrawableSceneMap.GetBoundFromScene(m_Drawable, nodeIndex);
	if (grmModel)
	{
		ImGui::SameLine();
		SlGui::IconButton("MESH", ICON_AM_MESH, IM_COL32(9, 178, 139, 255));
	}
	if (crBoneData)
	{
		ImGui::SameLine();
		SlGui::IconButton("BONE", ICON_AM_BONE);
	}
	if (lightAttr)
	{
		ImGui::SameLine();
		if (SlGui::IconButton("LIGHT", ICON_AM_LIGHT))
		{
			m_LightEditor.SelectLight(m_DrawableSceneMap.SceneNodeToLightAttr[nodeIndex]);
		}
	}
	if (phBound)
	{
		ImGui::SameLine();
		SlGui::IconButton("COLLISION", ICON_AM_COLLIDER);
	}
	ImGui::PopStyleVar(2); // ItemSpacing, FramePadding

	// Column: Visibility Eye
	ImGui::TableNextColumn();
	SlGui::IconButton("VIS", ICON_AM_EYE_ON);

	// Draw children
	if (opened)
	{
		// Draw children
		DrawSceneGraphRecurse(sceneNode->GetFirstChild());

		ImGui::TreePop();
	}

	// Draw sibling
	DrawSceneGraphRecurse(sceneNode->GetNextSibling());
}

void rageam::ModelSceneApp::DrawSceneGraph(const graphics::SceneNode* sceneNode)
{
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
	if (ImGui::BeginTable("SCENE_GRAPH_TABLE", 2))
	{
		ImGui::TableSetupColumn("Node", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Visibility", ImGuiTableColumnFlags_WidthFixed);
		DrawSceneGraphRecurse(sceneNode);
		ImGui::EndTable();
	}
	ImGui::PopStyleVar(3);
}

void rageam::ModelSceneApp::DrawSkeletonGraph()
{

}

void rageam::ModelSceneApp::DrawDrawableUI()
{
	if (!m_Drawable || !m_ModelState.IsEntitySpawned)
		return;

	m_LightEditor.Render(m_Drawable, m_ModelState.EntityWorld);

	if (m_SelectedNodeIndex != -1)
	{
		ConstString propertiesTitle = ImGui::FormatTemp("%s Properties###DRAWABLE_ATTR_PROPERTIES",
			m_Scene->GetNode(m_SelectedNodeIndex)->GetName());
		if (ImGui::Begin(propertiesTitle))
		{
			bool forceBone = false;
			ImGui::Checkbox("Force bone", &forceBone);
			ImGui::SameLine();
			ImGui::HelpMarker(
				"By default bone for a scene node is created if:\n"
				"- Node has skinning.\n"
				"- Node or one of node parent's has non-zero transform.\n"
				"This option allows to convert node into a bone regardless.");
		}
		ImGui::End();
	}

	if (m_SelectedNodeIndex != -1 && m_SelectedNodeAttr != SceneNodeAttr_None)
	{
		ConstString propertiesTitle = ImGui::FormatTemp("%s Properties###DRAWABLE_ATTR_PROPERTIES",
			SceneNodeAttrDisplay[m_SelectedNodeAttr]);
		if (ImGui::Begin(propertiesTitle))
		{
			switch (m_SelectedNodeAttr)
			{
			case SceneNodeAttr_Bone:
			{
				rage::crBoneData* boneData = m_DrawableSceneMap.GetBoneFromScene(m_Drawable, m_SelectedNodeIndex);

				// TODO: Edit bone tune
				// asset::DrawableBoneTune = m_Drawable
				u16 tag = boneData->GetBoneTag();
				static bool override = false;
				if (ImGui::Checkbox("Override", &override))
					if (!override) ImGui::BeginDisabled();
				ImGui::InputU16("Tag", &tag);
				if (!override) ImGui::EndDisabled();
				ImGui::HelpMarker("Bone tag is often overriden on vanilla game models (mostly PEDs). Use only if necessary.");

				SlGui::CategoryText("Transform");
				// TODO: Local / World

				rage::Vector3 translation = boneData->GetTranslation();;
				rage::Vector3 rotation = { 0.0f, 0.0f, 0.0f }; // TODO: QuatV -> Euler conversion
				rage::Vector3 scale = boneData->GetScale();
				ImGui::DragFloat3("Translation", (float*)&translation, 0.1f, -INFINITY, INFINITY, "%g");
				ImGui::DragFloat3("Rotation", (float*)&rotation, 0.1f, -INFINITY, INFINITY, "%g");
				ImGui::DragFloat3("Scale", (float*)&scale, 0.1f, -INFINITY, INFINITY, "%g");


				break;
			}
			default: break;
			}
		}
		ImGui::End();
	}

	if (ImGui::BeginTabBar("SCENE_DRAWABLE_TAB_BAR"))
	{
		if (ImGui::BeginTabItem("Graph"))
		{
			enum OutlineMode
			{
				OutlineMode_Scene,
				OutlineMode_Skeleton,
			};
			static constexpr ConstString s_OutlineModeDisplay[] = { "Scene", "Skeleton" };
			static int s_OutlineModeSelected = OutlineMode_Scene;
			ImGui::Combo("Graph Mode", &s_OutlineModeSelected, s_OutlineModeDisplay, IM_ARRAYSIZE(s_OutlineModeDisplay));

			graphics::SceneNode* rootNode = m_Scene->GetFirstNode();
			switch (s_OutlineModeSelected)
			{
			case OutlineMode_Scene:		DrawSceneGraph(rootNode);	break;
			case OutlineMode_Skeleton:	DrawSkeletonGraph();		break;
			}

			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Extras"))
		{
			rage::Vector3 scenePos = m_ScenePosition;
			if (ImGui::InputFloat3("Scene Position", (float*)&scenePos, "%g"))
			{
				m_ScenePosition = scenePos;
				UpdateScenePosition();
			}

			if (ImGui::Button("Pin scene to interior"))
			{
				scrInvoke([&]
					{
						SHV::Ped localPed = SHV::PLAYER::GET_PLAYER_PED(0);
						SHV::Hash room = SHV::INTERIOR::GET_ROOM_KEY_FROM_ENTITY(localPed);
						SHV::Interior interior = SHV::INTERIOR::GET_INTERIOR_FROM_ENTITY(localPed);
						SHV::INTERIOR::FORCE_ROOM_FOR_ENTITY((int)m_ModelState.EntityHandle, interior, room);
					});
			}
			ImGui::SameLine();
			ImGui::HelpMarker("Force entity into current player interior room.");

			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Debug"))
		{
			ImGui::Text("Entity Handle: %u", m_ModelState.EntityHandle);
			char ptrBuf[64];
			sprintf_s(ptrBuf, 64, "%p", m_Drawable);
			ImGui::InputText("Drawable Ptr", ptrBuf, 64, ImGuiInputTextFlags_ReadOnly);
			sprintf_s(ptrBuf, 64, "%p", m_ModelState.EntityPtr);
			ImGui::InputText("Entity Ptr", ptrBuf, 64, ImGuiInputTextFlags_ReadOnly);
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}

	return;

	//m_LightEditor.Render(entityWorld);

	//if (ImGui::CollapsingHeader("Render Options"))
	//{
	//	graphics::DebugRender& debugRender = GRenderContext->DebugRender;
	//	ImGui::Checkbox("Skeleton", &debugRender.bRenderSkeleton);
	//	ImGui::Checkbox("Bound Mesh", &debugRender.bRenderBoundMesh);
	//	ImGui::Text("Extents:");
	//	ImGui::Checkbox("Lod Group", &debugRender.bRenderLodGroupExtents);
	//	ImGui::Checkbox("Bound", &debugRender.bRenderBoundExtents);
	//	ImGui::Checkbox("Geometry", &debugRender.bRenderGeometryExtents);
	//}

	//if (ImGui::CollapsingHeader("Stats", ImGuiTreeNodeFlags_DefaultOpen))
	//{
	//	ImGui::Text("Dimensions: ( %.02f, %.02f, %.02f )",
	//		m_Dimensions.X, m_Dimensions.Y, m_Dimensions.Z);
	//	ImGui::Text("LODs: %u", m_NumLods);
	//	ImGui::Text("Models: %u", m_NumModels);
	//	ImGui::Text("Geometries: %u", m_NumGeometries);
	//	ImGui::Text("Vertices: %u", m_VertexCount);
	//	ImGui::Text("Triangles: %u", m_TriCount);
	//	ImGui::Text("Lights: %u", m_LightCount);
	//}

	////if (ImGui::CollapsingHeader("Skeleton", ImGuiTreeNodeFlags_DefaultOpen))
	//if (SlGui::CollapsingHeader(ICON_AM_SKELETON" Skeleton", ImGuiTreeNodeFlags_DefaultOpen))
	//{
	//	rage::crSkeletonData* skeleton = drawable->GetSkeletonData();
	//	if (skeleton)
	//	{
	//		ImGui::Text("Skeleton Bones");
	//		if (ImGui::BeginTable("TABLE_SKELETON", 6, ImGuiTableFlags_Borders))
	//		{
	//			ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_WidthFixed);
	//			ImGui::TableSetupColumn("Name");
	//			ImGui::TableSetupColumn("Parent");
	//			ImGui::TableSetupColumn("Sibling");
	//			ImGui::TableSetupColumn("Child");
	//			ImGui::TableSetupColumn("Tag");

	//			ImGui::TableHeadersRow();

	//			for (u16 i = 0; i < skeleton->GetBoneCount(); i++)
	//			{
	//				rage::crBoneData* bone = skeleton->GetBone(i);

	//				ImGui::TableNextRow();

	//				ImGui::TableNextColumn();
	//				ImGui::Text("%u", i);
	//				ImGui::TableNextColumn();
	//				ImGui::Text("%s", bone->GetName());
	//				ImGui::TableNextColumn();
	//				ImGui::Text("%i", bone->GetParentIndex());
	//				ImGui::TableNextColumn();
	//				ImGui::Text("%i", bone->GetNextIndex());
	//				ImGui::TableNextColumn();
	//				ImGui::Text("%i", (s16)skeleton->GetFirstChildBoneIndex(i));
	//				ImGui::TableNextColumn();
	//				ImGui::Text("%i", bone->GetBoneTag());
	//			}

	//			ImGui::EndTable();
	//		}


	//		std::function<void(rage::crBoneData*)> recurseBone;
	//		recurseBone = [&](const rage::crBoneData* bone)
	//			{
	//				if (!bone)
	//					return;

	//				ImGui::Indent();
	//				while (bone)
	//				{
	//					ImGui::BulletText("%s", bone->GetName());
	//					constexpr ImVec4 attrColor = ImVec4(0, 0.55, 0, 1);


	//					if (GRenderContext->DebugRender.bRenderSkeleton)
	//					{
	//						rage::Mat44V boneWorld = skeleton->GetBoneWorldTransform(bone) * entityWorld;
	//						Im3D::CenterNext();
	//						Im3D::TextBg(boneWorld.Pos, "<%s; Tag: %u>", bone->GetName(), bone->GetBoneTag());
	//					}

	//					ImGui::SameLine();
	//					ImGui::TextColored(attrColor, "Tag: %u", bone->GetBoneTag());

	//					//rage::Vec3V pos, scale;
	//					//rage::QuatV rot;
	//					//const rage::Mat44V& mtx = skeleton->GetBoneTransform(bone->GetIndex());
	//					//mtx.Decompose(&pos, &scale, &rot);

	//					//ImGui::TextColored(attrColor, "Trans: %f %f %f", pos.X(), pos.Y(), pos.Z());
	//					//ImGui::TextColored(attrColor, "Scale: %f %f %f", scale.X(), scale.Y(), scale.Z());
	//					//ImGui::TextColored(attrColor, "Rot: %f %f %f %f", rot.X(), rot.Y(), rot.Z(), rot.W());

	//					recurseBone(skeleton->GetFirstChildBone(bone->GetIndex()));

	//					bone = skeleton->GetBone(bone->GetNextIndex());
	//				}
	//				ImGui::Unindent();
	//			};

	//		rage::crBoneData* root = skeleton->GetBone(0);
	//		recurseBone(root);
	//	}
	//}
}

void rageam::ModelSceneApp::DrawStarBar()
{
	if (SlGui::BeginToolWindow(ICON_AM_STAR" StarBar")) // Scene Toolbar / CameraStar
	{
		if (SlGui::ToggleButton(ICON_AM_CAMERA_GIZMO" Camera", m_CameraEnabled))
			UpdateCamera();

		ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 0.2f);
		if (!m_CameraEnabled) ImGui::BeginDisabled();
		{
			if (ImGui::IsKeyPressed(ImGuiKey_O, false))
			{
				m_UseOrbitCamera = !m_UseOrbitCamera;
				UpdateCamera();
			}

			if (SlGui::ToggleButton(ICON_AM_ORBIT" Orbit", m_UseOrbitCamera))
				UpdateCamera();
			ImGui::ToolTip("Use orbit camera instead of free");

			if (SlGui::ToggleButton(ICON_AM_OBJECT " Isolate", m_IsolatedSceneActive))
			{
				UpdateScenePosition();
				ResetCameraPosition();
			}
			ImGui::ToolTip("Isolates scene model");

			// Separate toggle buttons from actions
			if (!m_CameraEnabled) ImGui::EndDisabled(); // Draw separator without opacity
			ImGui::Separator();
			if (!m_CameraEnabled) ImGui::BeginDisabled();

			if (SlGui::MenuButton(ICON_AM_HOME" Reset Cam"))
				ResetCameraPosition();

			if (SlGui::MenuButton(ICON_AM_PED_ARROW" Warp Ped"))
			{
				const rage::Vec3V pos = m_Camera->GetPosition();
				scrInvoke([=]
					{
						float groundZ = pos.Z();
						SHV::GAMEPLAY::GET_GROUND_Z_FOR_3D_COORD(pos.X(), pos.Y(), pos.Z(), &groundZ, FALSE);

						SHV::Ped ped = SHV::PLAYER::PLAYER_PED_ID();
						SHV::PED::SET_PED_COORDS_KEEP_VEHICLE(ped, pos.X(), pos.Y(), groundZ);
					});
			}
			ImGui::ToolTip("Teleports player ped on surface to camera position.");
		}

		if (!m_CameraEnabled) ImGui::EndDisabled();
		ImGui::PopStyleVar(1); // DisabledAlpha

		// World / Local gizmo switch
		ImGui::Separator();
		bool useWorld = Im3D::GetGizmoUseWorld();
		ConstString worldStr = ICON_AM_WORLD" World";
		ConstString localStr = ICON_AM_LOCAL" World";
		if (SlGui::ToggleButton(useWorld ? worldStr : localStr, useWorld))
			Im3D::SetGizmoUseWorld(useWorld);
		ImGui::ToolTip("Show edit gizmos in world or local space.");
		ImGui::Separator();
		if (ImGui::IsKeyPressed(ImGuiKey_Period, false)) // Hotkey switch
			Im3D::SetGizmoUseWorld(!useWorld);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1);
		if (ImGui::BeginMenu(ICON_AM_VISIBILITY" Overlay"))
		{
			SlGui::Checkbox("Light Outlines", &m_LightEditor.ShowLightOutlines);
			ImGui::SameLine();
			SlGui::Checkbox("Only Selected", &m_LightEditor.ShowOnlySelectedLightOutline);
			bool dummy;
			SlGui::Checkbox("Drawable Bound Box", &dummy);
			SlGui::Checkbox("Collision Mesh", &dummy);
			SlGui::Checkbox("Skeleton Bones", &dummy);

			ImGui::EndMenu();
		}
		ImGui::PopStyleVar(2); // WindowPadding, WindowBorderSize
	}
	SlGui::EndToolWindow();
}

void rageam::ModelSceneApp::UpdateScenePosition()
{
	m_ModelScene->SetEntityPos(GetEntityScenePos());

	scrInvoke([=]
		{
			bool display = !m_IsolatedSceneActive;
			SHV::UI::DISPLAY_HUD(display);
			SHV::UI::DISPLAY_RADAR(display);
		});
}

void rageam::ModelSceneApp::OnRender()
{
	std::unique_lock lock(m_Mutex);

	m_ModelScene->GetState(m_ModelState);
	DrawStarBar();

	// We don't want camera controls interference imgui
	if (m_Camera)
	{
		m_Camera->DisableControls(GImGui->HoveredWindow != nullptr);
	}

	if (ImGui::IsKeyPressed(ImGuiKey_RightBracket))
	{
		m_CameraEnabled = !m_CameraEnabled;
		UpdateCamera();
	}

	if (ImGui::Begin("Scene"))
	{
		constexpr ImVec2 buttonSize = { 90, 0 };

		// TODO: Broken
		bool isLoading = m_ModelState.IsLoading;

		if (isLoading) ImGui::BeginDisabled();
		if (ImGui::Button("Load", buttonSize))
		{
			m_ModelScene->SetupFor(m_AssetPath, GetEntityScenePos());
		}

		ImGui::SameLine();

		if (ImGui::Button("Unload", buttonSize))
		{
			m_ModelScene->CleanUp();
		}

		if (isLoading) ImGui::EndDisabled();

		if (isLoading)
		{
			// TODO: Report progress
			// Loading indicator
			int type = (int)(ImGui::GetTime() * 5) % 3;
			ImGui::Text("Loading%s", type == 0 ? "." : type == 1 ? ".." : "...");
		}

		if (m_ModelState.IsEntitySpawned)
		{
			DrawDrawableUI();
		}
	}
	ImGui::End(); // Scene
}

void rageam::ModelSceneApp::OnDrawableLoaded(const asset::DrawableAssetPtr& asset, gtaDrawable* drawable)
{
	// Note: this callback is called from game thread
	// std::unique_lock lock(m_Mutex); // TODO: This mutex causes deadlock...

	m_Drawable = drawable;
	m_Scene = asset->GetScene();
	m_DrawableSceneMap = std::move(asset->CompiledDrawableMap);

	m_SelectedNodeIndex = -1;
	m_SelectedNodeAttr = SceneNodeAttr_None;

	UpdateDrawableStats();
}

rageam::ModelSceneApp::ModelSceneApp()
{
	// Temporary solution until we have explorer integration
	wchar_t* path;
	if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Desktop, 0, 0, &path)))
	{
		m_AssetPath = path;
		m_AssetPath /= L"rageAm.idr";

		CoTaskMemFree(path);
	}

	m_ModelScene.Create();
	m_ModelScene->LoadCallback = [&](const asset::DrawableAssetPtr& asset, gtaDrawable* drawable)
		{
			OnDrawableLoaded(asset, drawable);
		};
}
#endif
