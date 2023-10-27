#ifdef AM_INTEGRATED

#include "modelscene.h"

#include "widgets.h"
#include "am/integration/integration.h"
#include "am/integration/shvthread.h"
#include "am/ui/font_icons/icons_am.h"
#include "am/ui/styled/slwidgets.h"
#include "scripthook/shvnatives.h"
#include "scripthook/shvtypes.h"

rageam::integration::DrawableStats rageam::integration::DrawableStats::ComputeFrom(gtaDrawable* drawable)
{
	using namespace rage;

	const spdAABB& bb = drawable->GetLodGroup().GetBoundingBox();
	const crSkeletonData* skeleton = drawable->GetSkeletonData();

	DrawableStats stats = {};
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

rageam::asset::DrawableAssetMap& rageam::integration::ModelScene::GetDrawableMap() const
{
	return m_Context.DrawableAsset->CompiledDrawableMap;
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
	static constexpr u32 ASSET_NAME_HASH = rage::joaat("amTestBedArchetype");

	// Make sure that lod distance is not smaller than drawable itself
	float lodDistance = m_Context.Drawable->GetBoundingSphere().GetRadius().Get();
	lodDistance += 100.0f;

	m_ArchetypeDef = std::make_shared<CBaseArchetypeDef>();
	m_ArchetypeDef->Name = ASSET_NAME_HASH;
	m_ArchetypeDef->AssetName = ASSET_NAME_HASH;
	m_ArchetypeDef->LodDist = lodDistance;
	m_ArchetypeDef->Flags = rage::ADF_STATIC | rage::ADF_BONE_ANIMS;

	m_GameEntity.Create();
	m_GameEntity->Spawn(m_Context.Drawable, m_ArchetypeDef, GetScenePosition());
}

void rageam::integration::ModelScene::WarpEntityToScenePosition()
{
	if (m_GameEntity)
		m_GameEntity->SetPosition(GetScenePosition());
}

void rageam::integration::ModelScene::OnDrawableCompiled()
{
	m_DrawableStats = DrawableStats::ComputeFrom(m_Context.Drawable.get());
	m_CompilerMessages.Destroy();
	m_CompilerProgress = 1.0;
	m_SelectedNodeIndex = -1;
	m_SelectedNodeAttr = SceneNodeAttr_None;
	m_LightEditor.Reset();
	m_MaterialEditor.Reset();

	CreateArchetypeDefAndSpawnGameEntity();
}

void rageam::integration::ModelScene::UpdateContextAndHotReloading()
{
	m_Context = {};

	// No scene was even requested
	if (!m_SceneGlue)
		return;

	std::unique_lock glueLock(m_SceneGlue->Mutex);

	// No drawable is currently loaded, skip
	if (!m_SceneGlue->HotDrawable)
		return;

	// Fill context
	asset::HotDrawableInfo info = m_SceneGlue->HotDrawable->GetInfo();
	m_Context.IsDrawableLoading = info.IsLoading;
	m_Context.DrawableAsset = info.DrawableAsset;
	m_Context.Drawable = info.Drawable;
	m_Context.TXDs = info.TXDs;
	// Entity will be NULL if drawable just compiled
	if (m_GameEntity)
	{
		m_Context.EntityHandle = m_GameEntity->GetHandle();
		m_Context.EntityWorld = m_GameEntity->GetWorldTransform();
		m_Context.EntityPtr = m_GameEntity->GetEntityPointer();
	}

	// Get applied flags and reset state
	AssetHotFlags hotFlags = m_SceneGlue->HotFlags;
	m_SceneGlue->HotFlags = AssetHotFlags_None;
	m_Context.HotFlags = hotFlags;

	// Drawable was successfully compiled, we can spawn entity now
	if (hotFlags & AssetHotFlags_DrawableCompiled)
	{
		OnDrawableCompiled();
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
			m_LightEditor.SelectLight(m_Context.GetDrawableMap()->SceneNodeToLightAttr[nodeIndex]);
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

void rageam::integration::ModelScene::DrawNodePropertiesUI(u16 nodeIndex) const
{
	graphics::SceneNode* sceneNode = m_Context.GetScene()->GetNode(nodeIndex);

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
				bool forceBone = false;
				ImGui::Checkbox("Force create bone", &forceBone);
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
				bool collider = false;
				ImGui::Checkbox("Create collider", &collider);

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
			ImGui::Combo("Graph Mode", &s_OutlineModeSelected, s_OutlineModeDisplay, IM_ARRAYSIZE(s_OutlineModeDisplay));

			graphics::SceneNode* rootNode = m_Context.GetScene()->GetFirstNode();
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
				WarpEntityToScenePosition();
			}

			if (ImGui::Button("Pin scene to interior"))
			{
				scrBegin();
				{
					SHV::Ped localPed = SHV::PLAYER::GET_PLAYER_PED(-1);
					SHV::Hash room = SHV::INTERIOR::GET_ROOM_KEY_FROM_ENTITY(localPed);
					SHV::Interior interior = SHV::INTERIOR::GET_INTERIOR_FROM_ENTITY(localPed);
					SHV::INTERIOR::FORCE_ROOM_FOR_ENTITY(m_Context.EntityHandle, interior, room);
				}
				scrEnd();
			}
			ImGui::SameLine();
			ImGui::HelpMarker("Force entity into current player interior room.");

			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Debug"))
		{
			ImGui::Text("Entity Handle: %i", m_Context.EntityHandle);
			char ptrBuf[64];
			sprintf_s(ptrBuf, 64, "%p", m_Context.Drawable.get());
			ImGui::InputText("Drawable Ptr", ptrBuf, 64, ImGuiInputTextFlags_ReadOnly);
			sprintf_s(ptrBuf, 64, "%p", m_Context.EntityPtr);
			ImGui::InputText("Entity Ptr", ptrBuf, 64, ImGuiInputTextFlags_ReadOnly);
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}

	if (m_SelectedNodeIndex != -1)
	{
		DrawNodePropertiesUI(m_SelectedNodeIndex);
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

void rageam::integration::ModelScene::OnRender()
{
	UpdateContextAndHotReloading();

	bool isLoading = m_Context.IsDrawableLoading;
	bool isSpawned = m_Context.EntityHandle != 0;
	bool needUnload = false;

	if (ImGui::Begin("Scene", 0, ImGuiWindowFlags_MenuBar))
	{
		// Menu Bar
		if (!isSpawned) ImGui::BeginDisabled();
		if (ImGui::BeginMenuBar())
		{
			// TODO: Should work via closing window?
			if (ImGui::MenuItem(ICON_AM_CANCEL" Unload"))
			{
				needUnload = true;
			}

			if (ImGui::MenuItem(ICON_AM_BALL" Material Editor"))
			{
				m_MaterialEditor.IsOpen = !m_MaterialEditor.IsOpen;
				// TODO: We can split material editor on Graphics & Physical materials
			}

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

				if (rage::Math::AlmostEquals(ImGui::GetScrollY(), ImGui::GetScrollMaxY()))
					ImGui::SetScrollY(ImGui::GetScrollMaxY());

			}
			ImGui::EndChild();

			// Align progress bar to bottom of window
			//ImGuiWindow* window = ImGui::GetCurrentWindow();
			//window->DC.CursorPos = window->WorkRect.GetBL() - ImVec2(0, ImGui::GetFrameHeight());
			//ImGui::ProgressBar(static_cast<float>(m_CompilerProgress));
		}

		if (isSpawned)
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

	if (isSpawned)
	{
		m_LightEditor.Render();
		m_MaterialEditor.Render();
	}
}

rageam::integration::ModelScene::ModelScene() : m_LightEditor(&m_Context), m_MaterialEditor(&m_Context)
{
	m_Context = {};
	m_DrawableStats = {};
}

void rageam::integration::ModelScene::Unload()
{
	m_GameEntity.Destroy();
	m_SceneGlue.Destroy();
	m_DrawableStats = {};
	m_Context = {};
	m_CompilerMessages.Clear();
	m_CompilerProgress = 0.0;
}

void rageam::integration::ModelScene::LoadFromPatch(ConstWString path)
{
	Unload();

	m_SceneGlue.Create();

	std::unique_lock glueLock(m_SceneGlue->Mutex);

	m_SceneGlue->HotDrawable = std::make_unique<asset::HotDrawable>(path);
	// Setup callback to display progress in UI
	m_SceneGlue->HotDrawable->CompileCallback = [&](ConstWString message, double progress)
		{
			std::unique_lock lock(m_CompilerMutex);
			m_CompilerMessages.Construct(String::ToAnsiTemp(message));
			m_CompilerProgress = progress;
		};
	m_SceneGlue->HotDrawable->Load();
}

void rageam::integration::ModelScene::SetIsolatedSceneOn(bool on)
{
	m_IsolatedSceneOn = on;
	ResetCameraPosition();
	WarpEntityToScenePosition();
}

void rageam::integration::ModelScene::ResetCameraPosition() const
{
	auto& camera = GameIntegration::GetInstance()->Camera;
	if (camera == nullptr)
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
