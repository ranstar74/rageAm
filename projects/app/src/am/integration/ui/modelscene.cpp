#include "am/asset/ui/assetwindowfactory.h"
#ifdef AM_INTEGRATED

#include "modelscene.h"

#include "widgets.h"
#include "am/integration/integration.h"
#include "am/ui/font_icons/icons_am.h"
#include "am/ui/slwidgets.h"

rageam::integration::DrawableStats rageam::integration::DrawableStats::ComputeFrom(gtaDrawable* drawable)
{
	using namespace rage;

	DrawableStats stats = {};
	if (!drawable)
		return stats;

	const spdAABB& bb = drawable->GetLodGroup().GetBoundingBox();
	const crSkeletonData* skeleton = drawable->GetSkeletonData().Get();

	stats.Dimensions = bb.Max - bb.Min;
	stats.Lights = drawable->GetLightCount();
	if (skeleton)
		stats.Bones = skeleton->GetBoneCount();

	for (int i = 0; i < LOD_COUNT; i++)
	{
		const rmcLod* lod = drawable->GetLodGroup().GetLod(i);
		if (!lod)
			break; // No more lods in lod group

		const grmModels& lodModels = lod->GetModels();

		stats.Lods++;
		stats.Models += lodModels.GetSize();
		for (auto& model : lodModels)
		{
			const grmGeometries& modelGeoms = model->GetGeometries();

			stats.Geometries += modelGeoms.GetSize();
			for (auto& geom : modelGeoms)
			{
				stats.Vertices += geom->GetVertexCount();
				stats.Triangles += geom->GetPrimitiveCount();
			}
		}
	}

	return stats;
}

const rageam::asset::DrawableAssetMap& rageam::integration::ModelScene::GetDrawableMap() const
{
	return *m_Context.DrawableAsset->CompiledDrawableMap;
}

rage::grmModel* rageam::integration::ModelScene::GetMeshAttr(u16 nodeIndex) const
{
	return GetDrawableMap().GetModelFromScene(GetDrawable(), nodeIndex);
}

rage::crBoneData* rageam::integration::ModelScene::GetBoneAttr(u16 nodeIndex) const
{
	return GetDrawableMap().GetBoneFromScene(GetDrawable(), nodeIndex);
}

rage::phBound* rageam::integration::ModelScene::GetBoundAttr(u16 nodeIndex) const
{
	return GetDrawableMap().GetBoundFromScene(GetDrawable(), nodeIndex);
}

CLightAttr* rageam::integration::ModelScene::GetLightAttr(u16 nodeIndex) const
{
	return GetDrawableMap().GetLightFromScene(GetDrawable(), nodeIndex);
}

rageam::Vec3V rageam::integration::ModelScene::GetScenePosition() const
{
	return m_IsolatedSceneOn ? SCENE_ISOLATED_POS : m_ScenePosition;
}

void rageam::integration::ModelScene::CreateArchetypeDefAndSpawnGameEntity()
{
	static constexpr u32 ASSET_NAME_HASH = rage::atStringHash("amTestBedArchetype");

	// Make sure that lod distance is not smaller than drawable itself
	float lodDistance = GetDrawable()->GetBoundingSphere().GetRadius().Get();
	lodDistance += 100.0f;

	m_ArchetypeDef = std::make_shared<CBaseArchetypeDef>();
	m_ArchetypeDef->Name = ASSET_NAME_HASH;
	m_ArchetypeDef->AssetName = ASSET_NAME_HASH;
	m_ArchetypeDef->PhysicsDictionary = ASSET_NAME_HASH;
	m_ArchetypeDef->LodDist = lodDistance;
	m_ArchetypeDef->Flags = FLAG_IS_TYPE_OBJECT | FLAG_IS_FIXED | FLAG_HAS_ANIM;

	m_GameEntity.Create(m_Context.Drawable, m_ArchetypeDef.get(), GetScenePosition());
	//m_GameEntity->Spawn(m_Context.Drawable, m_ArchetypeDef, GetScenePosition());
	//m_DrawableRender.Create();
	//m_DrawableRender->SetEntity(m_GameEntity);
}

void rageam::integration::ModelScene::WarpEntityToScenePosition()
{
	if (m_GameEntity)
		m_GameEntity->SetPosition(GetScenePosition());
}

void rageam::integration::ModelScene::OnDrawableCompiled()
{
	m_DrawableStats = DrawableStats::ComputeFrom(GetDrawable());
	m_CompilerMessages.Destroy();
	m_CompilerProgress = 1.0;

	if(m_ResetUIAfterCompiling)
	{
		m_SelectedNodeIndex = -1;
		m_SelectedNodeAttr = SceneNodeAttr_None;
		LightEditor.Reset();
		MaterialEditor.Reset();
	}

	CreateArchetypeDefAndSpawnGameEntity();
}

void rageam::integration::ModelScene::UpdateHotDrawableAndContext()
{
	m_Context = {};

	// No drawable is currently loaded, skip
	if (!m_HotDrawable)
		return;

	asset::HotDrawableInfo info = m_HotDrawable->UpdateAndApplyChanges();

	// Fill context
	m_Context.IsDrawableLoading = info.IsLoading;
	m_Context.DrawableAsset = info.DrawableAsset;
	m_Context.Drawable = info.Drawable;
	m_Context.MegaDictionary = info.MegaDictionary;
	m_Context.HotFlags = info.HotFlags;
	m_Context.HotDrawable = m_HotDrawable.get();

	// Entity will be NULL if drawable just compiled
	if (m_GameEntity)
	{
		m_Context.EntityHandle = m_GameEntity->GetEntityHandle();
		m_Context.EntityWorld = m_GameEntity->GetWorldTransform();
		m_Context.EntityPtr = m_GameEntity->GetEntityPointer();
	}
}

void rageam::integration::ModelScene::DrawSceneGraphRecurse(const graphics::SceneNode* sceneNode)
{
	if (!sceneNode)
		return;

	u16 nodeIndex = sceneNode->GetIndex();

	ImGui::TableNextRow();

	// Column: Node
	bool treeOpened;
	{
		ImGui::TableNextColumn();
		ConstString treeNodeName = ImGui::FormatTemp("%s###NODE_%u", sceneNode->GetName(), sceneNode->GetIndex());
		SlGuiTreeNodeFlags treeNodeFlags =
			SlGuiTreeNodeFlags_DefaultOpen;
		if (sceneNode->GetFirstChild() == nullptr)
			treeNodeFlags |= SlGuiTreeNodeFlags_NoChildren;

		bool selected = m_SelectedNodeIndex == nodeIndex;
		bool toggled;
		treeOpened = SlGui::GraphTreeNode(treeNodeName, selected, toggled, treeNodeFlags);
		if (selected)
			m_SelectedNodeIndex = nodeIndex;

		// Attribute buttons
		auto attrButton = [&](pVoid attrData, eSceneNodeAttr attr, ConstString icon, ImU32 col = IM_COL32_WHITE)
			{
				if (!attrData)
					return false;
				ImGui::SameLine();
				bool pressed = SlGui::IconButton(icon, col);
				if (pressed)
				{
					m_SelectedNodeIndex = nodeIndex;
					m_SelectedNodeAttr = attr;
					m_JustSelectedNodeAttr = attr;
				}
				return pressed;
			};
		ImGui::SameLine(); ImGui::Dummy(ImVec2(2, 0)); // Spacing after text
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 4));
		// Mesh
		rage::grmModel* grmModel = GetMeshAttr(nodeIndex);
		if (attrButton(grmModel, SceneNodeAttr_Mesh, ICON_AM_MESH, IM_COL32(9, 178, 139, 255))) // Green color
		{

		}
		// Bone
		rage::crBoneData* crBoneData = GetBoneAttr(nodeIndex);
		if (attrButton(crBoneData, SceneNodeAttr_Bone, ICON_AM_BONE))
		{

		}
		// Collision
		rage::phBound* phBound = GetBoundAttr(nodeIndex);
		if (attrButton(phBound, SceneNodeAttr_Collision, ICON_AM_COLLIDER))
		{

		}
		// Light
		CLightAttr* lightAttr = GetLightAttr(nodeIndex);
		if (attrButton(lightAttr, SceneNodeAttr_Light, ICON_AM_LIGHT))
		{
			LightEditor.SelectLight(GetDrawableMap().SceneNodeToLightAttr[nodeIndex]);
		}
		ImGui::PopStyleVar(2); // ItemSpacing, FramePadding
	}

	// Column: Visibility Eye
	ImGui::TableNextColumn();
	SlGui::IconButton(ICON_AM_EYE_ON);

	// Draw children
	if (treeOpened)
	{
		DrawSceneGraphRecurse(sceneNode->GetFirstChild());
		ImGui::TreePop();
	}

	// Draw sibling
	DrawSceneGraphRecurse(sceneNode->GetNextSibling());
}

void rageam::integration::ModelScene::DrawSceneGraph(const graphics::SceneNode* sceneNode)
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

void rageam::integration::ModelScene::DrawSkeletonGraph()
{

}

void rageam::integration::ModelScene::DrawNodePropertiesUI(u16 nodeIndex)
{
	graphics::SceneNode* sceneNode = m_Context.DrawableAsset->GetScene()->GetNode(nodeIndex);
	asset::ModelTune& modelTune = *m_Context.DrawableAsset->GetDrawableTune().Lods.Models.Get(nodeIndex);

	ConstString title = ImGui::FormatTemp(
		"%s Properties###DRAWABLE_ATTR_PROPERTIES", sceneNode->GetName());

	auto beginAttrTabItem = [&](pVoid attr, eSceneNodeAttr attrType, ConstString icon) -> bool
		{
			if (!attr)
				return false;

			ImGuiTabItemFlags flags = ImGuiTabItemFlags_None;
			if (m_JustSelectedNodeAttr == attrType)
				flags = ImGuiTabItemFlags_SetSelected;

			return ImGui::BeginTabItem(icon, 0, flags);
		};

	if (ImGui::Begin(title))
	{
		if (ImGui::BeginTabBar("PROPERTIES_TAB_BAR"))
		{
			// Node Properties
			if (ImGui::BeginTabItem("Common"))
			{
				if (ImGui::Checkbox("Force create bone", &modelTune.CreateBone))
					m_AssetTuneChanged = true;
				ImGui::SameLine();
				ImGui::HelpMarker(
					"Note: we are referring to scene objects as 'Nodes'\n"
					"By default bone for a scene node is created if:\n"
					"- Node has skinning.\n"
					"- Node or one of node parent's has non-identity transform (Reset XForm in 3Ds Max sets it to identity).\n"
					"This option allows to convert node into a bone regardless.\n"
					"It is important to understand that in rage skeleton is often used to set transformation (e.g. for animations) to a mesh\n"
					"    (or grmModel, technically speaking) and doesn't require mesh having skinning.\n"
					"Common examples of using a bone:\n"
					"- Bone world & local position / rotation can be accessed from script using natives\n"
					"- Game entities can be attached to a bone (including ropes and cameras)\n"
					"- Animation (requires YCD or scripting)\n");

				// TODO: We can create physical material for regular materials
				// TODO: Shouldn't be visible if already a collider
				if (ImGui::Checkbox("Force create collider", &modelTune.UseAsBound))
					m_AssetTuneChanged = true;

				ImGui::EndTabItem();
			}

			// Mesh
			rage::grmModel* grmModel = GetMeshAttr(nodeIndex);
			if (beginAttrTabItem(grmModel, SceneNodeAttr_Mesh, ICON_AM_MESH" Mesh"))
			{
				if (SlGui::CollapsingHeader("Render Flags"))
				{
					u32 renderFlags = grmModel->GetRenderFlags();
					if (integration::widgets::EnumFlags<rage::grcRenderFlags>("RENDER_FLAGS", "LF", &renderFlags))
					{
						ImGui::CheckboxFlags("Visibility", &renderFlags, rage::RF_VISIBILITY);
						ImGui::CheckboxFlags("Shadows", &renderFlags, rage::RF_SHADOWS);
						ImGui::CheckboxFlags("Reflections", &renderFlags, rage::RF_REFLECTIONS);
						ImGui::CheckboxFlags("Mirror", &renderFlags, rage::RF_MIRROR);
					}
					grmModel->SetRenderFlags(renderFlags);
				}

				ImGui::EndTabItem();
			}

			// Bone
			rage::crBoneData* crBoneData = GetBoneAttr(nodeIndex);
			if (beginAttrTabItem(crBoneData, SceneNodeAttr_Bone, ICON_AM_BONE" Bone"))
			{
				// TODO: Edit bone tune
				// asset::DrawableBoneTune = m_Drawable
				u16 tag = crBoneData->GetBoneTag();
				static bool override = false;
				ImGui::Checkbox("Override", &override);
				ImGui::SameLine();
				ImGui::HelpMarker("Bone tag is often overriden on vanilla game models (mostly PEDs). Use only if necessary.");
				if (!override) ImGui::BeginDisabled();
				ImGui::InputU16("Tag", &tag);
				if (!override) ImGui::EndDisabled();
				ImGui::SameLine();
				ImGui::HelpMarker("Tag (or ID) is bone name hash (ELF Hash wrapped to U16 range), used to quickly look up a bone in hash set.");

				SlGui::CategoryText("Local Transform");
				// TODO: Local / World, Euler

				ImGui::BeginDisabled();
				rage::Vector3 translation = crBoneData->GetTranslation();;
				rage::Vector3 rotation = { 0.0f, 0.0f, 0.0f }; // TODO: QuatV -> Euler conversion
				rage::Vector3 scale = crBoneData->GetScale();
				ImGui::DragFloat3("Translation", (float*)&translation, 0.1f, -INFINITY, INFINITY, "%g");
				ImGui::DragFloat3("Rotation", (float*)&rotation, 0.1f, -INFINITY, INFINITY, "%g");
				ImGui::DragFloat3("Scale", (float*)&scale, 0.1f, -INFINITY, INFINITY, "%g");
				ImGui::EndDisabled();

				ImGui::EndTabItem();
			}

			// Collision
			rage::phBound* phBound = GetBoundAttr(nodeIndex);
			if (beginAttrTabItem(phBound, SceneNodeAttr_Collision, ICON_AM_COLLIDER" Collision"))
			{

				ImGui::EndTabItem();
			}

			//// Light
			//CLightAttr* lightAttr = GetLightAttr(nodeIndex);
			//if (beginAttrTabItem(lightAttr, SceneNodeAttr_Light, ICON_AM_LIGHT" Light"))
			//{
			//	if (ImGui::Button("Select"))
			//	{
			//		m_LightEditor.SelectLight(m_DrawableSceneMap.SceneNodeToLightAttr[nodeIndex]);
			//	}
			//	ImGui::EndTabItem();
			//}

			ImGui::EndTabBar();
		}
	}
	ImGui::End();
}

void rageam::integration::ModelScene::DrawDrawableUI()
{
	m_JustSelectedNodeAttr = SceneNodeAttr_None;

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
			ImGui::SetNextItemWidth(ImGui::GetFontSize() * 5.0f);
			ImGui::Combo("Graph Mode", &s_OutlineModeSelected, s_OutlineModeDisplay, IM_ARRAYSIZE(s_OutlineModeDisplay));

			graphics::SceneNode* rootNode = m_Context.DrawableAsset->GetScene()->GetFirstNode();
			switch (s_OutlineModeSelected)
			{
			case OutlineMode_Scene:		DrawSceneGraph(rootNode);	break;
			case OutlineMode_Skeleton:	DrawSkeletonGraph();		break;
			}

			ImGui::EndTabItem();
		}

		//if (ImGui::BeginTabItem("Extras"))
		//{
		//	rage::Vector3 scenePos = m_ScenePosition;
		//	if (ImGui::InputFloat3("Scene Position", (float*)&scenePos, "%g"))
		//	{
		//		m_ScenePosition = scenePos;
		//		WarpEntityToScenePosition();
		//	}

		//	if (ImGui::Button("Pin scene to interior"))
		//	{
		//		//scrBegin();
		//		{
		//			//SHV::Ped localPed = SHV::PLAYER::GET_PLAYER_PED(-1);
		//			//SHV::Hash room = SHV::INTERIOR::GET_ROOM_KEY_FROM_ENTITY(localPed);
		//			//SHV::Interior interior = SHV::INTERIOR::GET_INTERIOR_FROM_ENTITY(localPed);
		//			//SHV::INTERIOR::FORCE_ROOM_FOR_ENTITY(m_Context.EntityHandle, interior, room);
		//		}
		//		//scrEnd();
		//	}
		//	ImGui::SameLine();
		//	ImGui::HelpMarker("Force entity into current player interior room.");

		//	ImGui::EndTabItem();
		//}

		//if (ImGui::BeginTabItem("Debug"))
		//{
		//	ImGui::Text("Entity Handle: %i", m_Context.EntityHandle);
		//	char ptrBuf[64];
		//	sprintf_s(ptrBuf, 64, "%p", m_Context.Drawable.get());
		//	ImGui::InputText("Drawable Ptr", ptrBuf, 64, ImGuiInputTextFlags_ReadOnly);
		//	sprintf_s(ptrBuf, 64, "%p", m_Context.EntityPtr);
		//	ImGui::InputText("Entity Ptr", ptrBuf, 64, ImGuiInputTextFlags_ReadOnly);
		//	ImGui::EndTabItem();
		//}

		if (ImGui::BeginTabItem("TXDs"))
		{
			ImGui::HelpMarker(
				"Here you can see available texture dictionaries, including embed one\n"
				"Press on button to open TXD editor\n"
				"You can add shared dictionaries by placing them in workspace folder with '.ws' extension\n"
				"NOTE: Game TXD linking system is slightly different, it is either done via .gtxd config file or in an archetype", 
				"Quick Info ");
			ImGui::BeginChild("TXD_LIST");
			int txdIndex = 0;
			for (asset::HotDictionary& hotDict : m_Context.HotDrawable->GetHotDictionaries())
			{
				if (hotDict.IsOrphan)
					continue;

				ConstString name = hotDict.TxdName;
				if (ImGui::ButtonEx(ImGui::FormatTemp("%s###TXD_%i", name, txdIndex++), ImVec2(-1, 0)))
				{
					const asset::TxdAssetPtr& txdAsset = m_Context.HotDrawable->GetTxdAsset(hotDict.TxdAssetPath);
					ui::AssetWindowFactory::OpenNewOrFocusExisting(txdAsset);
				}
			}
			ImGui::EndChild();
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	if (m_SelectedNodeIndex != -1)
	{
		DrawNodePropertiesUI(m_SelectedNodeIndex);
	}
}

void rageam::integration::ModelScene::OnRender()
{
	m_AssetTuneChanged = false;

	UpdateHotDrawableAndContext();

	if (m_Context.HotFlags & AssetHotFlags_DrawableUnloaded)
	{
		Unload(true);
	}

	// Drawable was successfully compiled, we can spawn entity now
	if (m_Context.HotFlags & AssetHotFlags_DrawableCompiled)
	{
		OnDrawableCompiled();
	}

	bool isLoading = m_Context.IsDrawableLoading;
	bool isSpawned = m_Context.EntityHandle != 0;
	bool needUnload = false;

	if (ImGui::Begin("Scene", 0, ImGuiWindowFlags_MenuBar))
	{
		// Menu Bar
		if (!isSpawned) ImGui::BeginDisabled();
		if (ImGui::BeginMenuBar())
		{
			if(ImGui::MenuItem(ICON_AM_SAVE" Save"))
			{
				m_Context.DrawableAsset->ParseFromGame(GetDrawable());
				if(!m_Context.DrawableAsset->SaveConfig())
				{
					AM_ERRF("ModelScene::OnRender() -> Failed to save config...");
				}
			}

			// TODO: Should work via closing window?
			if (ImGui::MenuItem(ICON_AM_CANCEL" Unload"))
			{
				needUnload = true;
			}

			//if (ImGui::MenuItem(ICON_AM_BALL" Materials"))
			//{
			//	MaterialEditor.IsOpen = !MaterialEditor.IsOpen;
			//	// TODO: We can split material editor on Graphics & Physical materials
			//}

			ImGui::EndMenuBar();
		}
		if (!isSpawned) ImGui::EndDisabled();

		// Display progress if loading
		if (isLoading)
		{
			std::unique_lock lock(m_CompilerMutex);

			// Leave space for progress bar
			ImGui::ProgressBar(static_cast<float>(m_CompilerProgress));
			float childHeight = 0;
				//ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeight() - ImGui::GetStyle().FramePadding.y * 2;
			if (ImGui::BeginChild("##COMPILER_PROGRESS", ImVec2(0, childHeight)))
			{
				for (ConstString msg : m_CompilerMessages)
				{
					ImGui::Text(msg);
				}

				if (rage::AlmostEquals(ImGui::GetScrollY(), ImGui::GetScrollMaxY()))
					ImGui::SetScrollY(ImGui::GetScrollMaxY());

			}
			ImGui::EndChild();

			// Align progress bar to bottom of window
			//ImGuiWindow* window = ImGui::GetCurrentWindow();
			//window->DC.CursorPos = window->WorkRect.GetBL() - ImVec2(0, ImGui::GetFrameHeight());
			//ImGui::ProgressBar(static_cast<float>(m_CompilerProgress));
		}

		if (isSpawned && !isLoading)
		{
			DrawDrawableUI();
		}
	}
	ImGui::End(); // Scene

	if (needUnload)
	{
		Unload();
		return;
	}

	if (isSpawned && !isLoading)
	{
		LightEditor.Render();
		MaterialEditor.Render();
	}

	// Recompile drawable after changing tune
	if (m_AssetTuneChanged)
	{
		//// We must save existing changes (materials/lights)
		//m_Context.DrawableAsset->ParseFromGame(m_Context.Drawable.get());

		//m_DrawableRender.Release();
		//m_GameEntity.Release();
		//m_DrawableStats = {};
		//m_Context = {};
		//m_CompilerMessages.Clear();
		//m_CompilerProgress = 0.0;

		//m_ResetUIAfterCompiling = false;
		//m_SceneGlue->HotDrawable->LoadAndCompile(true);
	}
}

rageam::integration::ModelScene::ModelScene() : LightEditor(&m_Context), MaterialEditor(&m_Context)
{
	m_Context = {};
	m_DrawableStats = {};
}

void rageam::integration::ModelScene::Unload(bool keepHotDrawable)
{	
	//m_DrawableRender.Release();

	m_CompilerMessages.Clear();
	m_CompilerProgress = 0.0;

	m_DrawableStats = {};
	m_Context = {};
	m_GameEntity = nullptr;

	if (!keepHotDrawable)
		m_HotDrawable = nullptr;
}

void rageam::integration::ModelScene::LoadFromPatch(ConstWString path)
{
	Unload(true);

	if (!m_HotDrawable || m_HotDrawable->GetPath() != path)
	{
		m_HotDrawable = nullptr;
		m_HotDrawable = std::make_unique<asset::HotDrawable>(path);
	}

	// Setup callback to display progress in UI
	m_HotDrawable->CompileCallback = [&](ConstWString message, double progress)
		{
			std::unique_lock lock(m_CompilerMutex);
			m_CompilerMessages.Construct(String::ToAnsiTemp(message));
			m_CompilerProgress = progress;
		};

	m_ResetUIAfterCompiling = true;
	m_HotDrawable->LoadAndCompileAsync();
}

void rageam::integration::ModelScene::SetIsolatedSceneOn(bool on)
{
	m_IsolatedSceneOn = on;
	ResetCameraPosition();
	WarpEntityToScenePosition();
}

void rageam::integration::ModelScene::ResetCameraPosition() const
{
	ICameraComponent* camera = ICameraComponent::GetActiveCamera();
	if (!camera)
		return;

	rage::Vec3V camPos;
	rage::Vec3V targetPos;
	rage::Vec3V scenePos = m_IsolatedSceneOn ? SCENE_ISOLATED_POS : m_ScenePosition;

	// Set view on drawable center if was spawned, otherwise just target scene position
	if (m_Context.Drawable)
	{
		rage::rmcLodGroup& lodGroup = m_Context.Drawable->GetLodGroup();
		auto& bb = lodGroup.GetBoundingBox();
		auto& bs = lodGroup.GetBoundingSphere();

		camPos = scenePos;
		// Shift camera away to fully see bounding sphere + add light padding
		camPos += rage::VEC_BACK * bs.GetRadius() * 1.5f;
		camPos += rage::VEC_UP * bs.GetRadius() * 0.45f;
		// Entities are spawned with bottom of bounding box aligned to specified coord
		targetPos = scenePos + rage::VEC_UP * bb.Height() * rage::S_HALF;
	}
	else
	{
		camPos = scenePos + rage::VEC_FORWARD + rage::VEC_UP;
		targetPos = scenePos;
	}

	camera->SetPosition(camPos);
	camera->LookAt(targetPos);
}

#endif
