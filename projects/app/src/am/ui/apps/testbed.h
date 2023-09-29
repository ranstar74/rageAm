#pragma once

#pragma comment(lib, "dxguid.lib")

#include <d3d11.h>
#include <d3d11shader.h>
#include <d3dcompiler.h>

#include "am/asset/factory.h"
#include "am/asset/types/drawable.h"
#include "am/graphics/buffereditor.h"
#include "am/graphics/meshsplitter.h"
#include "am/graphics/scene.h"
#include "am/graphics/render/engine.h"
#include "am/graphics/texture/imagefit.h"

#include "am/ui/extensions.h"
#include "am/ui/font_icons/icons_am.h"
#include "am/ui/styled/slgui.h"
#include "am/ui/styled/slwidgets.h"
#include "am/xml/doc.h"
#include "game/archetype.h"
#include "game/drawable.h"
#include "rage/atl/datahash.h"
#include "rage/creature/hash.h"
#include "rage/file/watcher.h"
#include "rage/framework/archetype.h"
#include "rage/grcore/fvfchannels.h"
#include "rage/grcore/effect/effect.h"
#include "rage/grcore/texture/texturedx11.h"
#include "rage/grm/geometry.h"
#include "rage/grm/model.h"
#include "rage/math/math.h"
#include "rage/math/vec.h"
#include "rage/paging/builder/builder.h"
#include "rage/physics/bounds/geometry.h"
#include "rage/physics/inst.h"
#include "rage/physics/bounds/box.h"
#include "rage/physics/bounds/composite.h"
#include "rage/streaming/assetstore.h"
#include "rage/streaming/streaming.h"
#ifndef AM_STANDALONE
#include "am/integration/shvthread.h"
#include "scripthook/shvnatives.h"
#include "am/integration/hooks/streaming.h"
#include "am/integration/memory/hook.h"
#endif
//#define PHTEST

#ifdef PHTEST
static u64 entity = 0x177FD580;
#endif

// #define SAVE_DRAWABLE

namespace rageam::ui
{
	ID3D11ShaderResourceView* (*gImpl_GetView_Hook)(rage::grcTextureDX11*);

	inline ID3D11ShaderResourceView* GetView_Hook(rage::grcTextureDX11* inst)
	{
		if (strcmp(inst->GetName(), "head_diff_000_a_whi") == 0)
		{
			int i = 0;
		}

		return gImpl_GetView_Hook(inst);
	}

	class TestbedApp : public App
	{
		//static inline bool (*gImpl_ReadSettings)();
		//static inline bool (*gImpl_ReadBlob)(u64, const char*);
		//static bool aImpl_ReadBlob(u64 a1, const char* path)
		//{
		//	return gImpl_ReadBlob(a1, "rageam/visualsettings.dat");
		//}


		//rage::fiDirectoryWatcher watcher;

	public:
		//void VsHook()
		//{
		//	static std::atomic_bool needRead = false;

		//	static gmAddress readBlobFn = gmAddress::Scan(
		//		"48 89 5C 24 08 55 56 57 48 83 EC 20 48 8B C2", "ReadBlock");
		//	static gmAddress readSettingsFn = gmAddress::Scan(
		//		"48 83 EC 28 48 8D 15 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 84", "ReadSettings");

		//	static bool init = false;
		//	if (!init)
		//	{
		//		Hook::Create(readBlobFn, aImpl_ReadBlob, &gImpl_ReadBlob, true);
		//		gImpl_ReadSettings = readSettingsFn.To<decltype(gImpl_ReadSettings)>();
		//		watcher.SetEntry("rageam");
		//		watcher.OnChangeCallback = []
		//		{
		//			needRead = true;
		//		};

		//		init = true;
		//	}

		//	if (needRead)
		//	{
		//		gImpl_ReadSettings();
		//		needRead = false;
		//	}

		//	/*	lastReadSuccess =;

		//		static bool lastReadSuccess = false;
		//		if (ImGui::Button("Reload visualsettings.dat"))
		//		{

		//		}
		//		ImGui::Text("Loaded: %s", lastReadSuccess ? "yes" : "no");
		//		ImGui::Text("ReadBlob: %p", readBlobFn.To<void*>());
		//		ImGui::Text("ReadSettings: %p", readSettingsFn.To<void*>());*/
		//}

#define TEST_MODEL_NAME "prop_metal_plates02"
//#define TEST_MODEL_NAME "vb_36_cablemesh1926_hvlit"

		rage::rmcDrawable* oldDrawable;
		rage::phArchetype* oldArchetype;
		~TestbedApp() override
		{
#ifndef AM_STANDALONE

#ifdef SAVE_DRAWABLE
			using namespace rage;

			// Reset
			static char drawableName[64] = TEST_MODEL_NAME;
			strLocalIndex slot;
			fwAssetStore<rmcDrawable, fwAssetDef<rmcDrawable>>* module = (decltype(module))hooks::Streaming::GetModule("ydr");
			module->GetSlotIndexByName(slot, drawableName);
			module->GetDef(slot)->m_pObject = oldDrawable;




#endif

#ifdef PHTEST
			// Ph inst
			{
				static gmAddress addr = gmAddress::Scan("E8 ?? ?? ?? ?? 48 8B D5 0F 28 D6").GetCall();
				static phInst* (*getPh)(u64) = addr.To<decltype(getPh)>();

				phInst* phInst = getPh(entity);
				phInst->m_Archetype.SetNoRef(oldArchetype);
			}
#endif





#endif
		}

		void OnStart() override
		{
#ifndef AM_STANDALONE
#ifdef SAVE_DRAWABLE
			using namespace rage;

			static char drawableName[64] = TEST_MODEL_NAME;
			rage::strLocalIndex slot;
			fwAssetStore<rmcDrawable, fwAssetDef<rmcDrawable>>* module = (decltype(module))hooks::Streaming::GetModule("ydr");
			module->GetSlotIndexByName(slot, drawableName);


			oldDrawable = module->GetDef(slot)->m_pObject;

			//auto addr = gmAddress::Scan("48 8B 41 78 C3");

			//Hook::Create(addr, GetView_Hook, &gImpl_GetView_Hook, true);
#endif SAVE_DRAWABLE
#ifdef PHTEST
			{
				static gmAddress addr = gmAddress::Scan("E8 ?? ?? ?? ?? 48 8B D5 0F 28 D6").GetCall();
				static phInst* (*getPh)(u64) = addr.To<decltype(getPh)>();

				phInst* phInst = getPh(entity);
				oldArchetype = phInst->m_Archetype.Get();
			}
#endif

#endif
		}







		rage::atArray<rage::atString> m_MaterialNames;

#ifndef AM_STANDALONE
		using fwTxdStore = rage::fwAssetStore<rage::grcTextureDictionary, rage::fwAssetDef<rage::grcTextureDictionary>>;
		static fwTxdStore* GetTxdStore()
		{
			static fwTxdStore* txdStore = static_cast<fwTxdStore*>(hooks::Streaming::GetModule("ytd"));
			return txdStore;
		}
#endif

		rage::rmcDrawable* m_Drawable;
		int m_SelectedMaterialIndex = 0;

#ifndef AM_STANDALONE
		// TODO: This would be useful when we implement .itd search
		//rage::atArray<rage::strLocalIndex> m_TextureSearchResults;
		rage::strLocalIndex m_SearchYtdSlot = rage::INVALID_STR_INDEX;
		rage::atArray<rage::grcTexture*> m_SearchTextures;

		void SearchTexture(ImmutableString searchText)
		{
			m_SearchTextures.Clear();

			// Search string is in format:
			// Ytd_Name/Texture_Name
			int separatorIndex = searchText.IndexOf('/', true);
			if (separatorIndex == -1)
				return;

			// For now we only support asset store and it stores only YTD name hash
			u32 searchDictHash = rage::atDataHash(searchText, separatorIndex); // String View would help here
			ConstString searchTexture = searchText.Substring(separatorIndex + 1);

			fwTxdStore* txdStore = GetTxdStore();
			m_SearchYtdSlot = txdStore->FindSlotFromHashKey(searchDictHash);
			if (m_SearchYtdSlot == rage::INVALID_STR_INDEX)
				return;

			auto dict = static_cast<rage::grcTextureDictionary*>(txdStore->GetPtr(m_SearchYtdSlot));
			// TODO: YTD is not streamed, what do we do? There's request async placement function I think
			if (!dict)
				return;

			for (u16 i = 0; i < dict->GetSize(); i++)
			{
				rage::grcTexture* texture = dict->GetValueAt(i);
				if (!String::IsNullOrEmpty(searchTexture) && ImmutableString(texture->GetName()).StartsWith(searchTexture))
					continue;

				m_SearchTextures.Add(texture);
			}
		}

		rage::grcTexture* DrawTexturePicker(ConstString id, const rage::grcTexture* currentTexture)
		{
			// TODO: Would be great to add some button to open texture explorer in style or file explorer,
			// with dynamically generated thumbnails for TXDs 

			auto getHintFn = [this](int index, const char** hint, ImTextureID* icon, ImVec2* iconSize, float* iconScale)
				{
					rage::grcTextureDX11* texture = (rage::grcTextureDX11*)m_SearchTextures.Get(index);
					*hint = texture->GetName();
					*icon = texture->GetShaderResourceView();

					constexpr float maxIconSize = 16.0f;
					auto [width, height] =
						Resize(texture->GetWidth(), texture->GetHeight(), maxIconSize, maxIconSize, texture::ScalingMode_Fit);
					*iconSize = ImVec2(width, height);
				};

			static auto compareHintFn = [](const char* text, const char* hint)
				{
					// We don't do search on some static collection but do it dynamically so this function is not needed
					return true;
				};

			ConstString defaultName = currentTexture ? currentTexture->GetName() : "";
			SlPickerState pickerState = SlGui::InputPicker(id, defaultName, m_SearchTextures.GetSize(), getHintFn, compareHintFn);

			if (pickerState.HintAccepted)
				return m_SearchTextures[pickerState.HintIndex];

			// Picker is not active
			if (!pickerState.NeedHints)
			{
				return nullptr;
			}

			// TODO: Would make for background thead or simply max execution time for search loop, maybe coroutine?
			if (pickerState.SearchChanged)
				SearchTexture(pickerState.Search);

			return nullptr;
		}

		void DrawMaterialList(const rage::grmShaderGroup* shaderGroup)
		{
			// TODO: Sphere Ball Preview & Preview resizing (ItemDragged?)

			if (!ImGui::BeginChild("MaterialEditor_List"))
			{
				ImGui::EndChild();
				return;
			}
			SlGui::ShadeItem(SlGuiCol_Bg);

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
			for (u16 i = 0; i < shaderGroup->GetShaderCount(); i++)
			{
				rage::grmShader* shader = shaderGroup->GetShader(i);
				rage::grcEffect* effect = shader->GetEffect();

				ConstString effectName = effect->GetName();
				if (m_MaterialNames.Any())
					effectName = m_MaterialNames[i].GetCStr();
				// We must ensure that nodes will have unique name
				ConstString nodeName = ImGui::FormatTemp("%s###%s_%i", effectName, effectName, i);

				bool selected = m_SelectedMaterialIndex == i;
				bool toggled;
				SlGui::TreeNode(nodeName, selected, toggled, ImTextureID(nullptr), SlGuiTreeNodeFlags_NoChildren);
				if (selected)
					m_SelectedMaterialIndex = i;
			}
			ImGui::PopStyleVar(); // Item_Spacing
			ImGui::EndChild(); // MaterialEditor_List
		}

		void DrawMaterialProperties(const rage::grmShaderGroup* shaderGroup)
		{
			if (!SlGui::BeginPadded("MaterialEditor_Properties_Padding", ImVec2(6, 6)))
			{
				SlGui::EndPadded();
				return;
			}

			rage::grmShader* shader = shaderGroup->GetShader(m_SelectedMaterialIndex);

			SlGui::PushFont(SlFont_Medium);
			SlGui::CategoryText("Shader");
			SlGui::PopFont();

			{
				static constexpr ConstString CHOOSE_SHADER_POPUP = "ChooseShaderPopup";

				// TODO: Move hook code out
				static gmAddress findEffectFn = gmAddress::Scan("48 89 5C 24 08 4C 63 1D ?? ?? ?? ?? 48");
				static rage::grcEffect** g_Effects = findEffectFn.GetRef(0xC + 0x3).To<rage::grcEffect**>();

				// Option to show all existing shaders in game or pre-defined list like in zModeler
				static bool showAll = false;

				// Shader Picker
				rage::grcEffect* effect = shader->GetEffect();
				ConstString pickButtonText = ImGui::FormatTemp("%s  %s", ICON_AM_COLOR_PICKER, effect->GetName());
				if (ImGui::Button(pickButtonText))
					ImGui::OpenPopup(CHOOSE_SHADER_POPUP);
				ImGui::SetItemTooltip("Choose a new shader");

				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
				ImGui::SetNextWindowSize(ImVec2(190, 0));
				if (ImGui::BeginPopup(CHOOSE_SHADER_POPUP))
				{
					// TODO: Can we define those in XML or JSON?
					ConstString newEffectName = nullptr;
					if (ImGui::BeginMenu("Generic"))
					{
						if (ImGui::BeginMenu("Parallax"))
						{
							ImGui::EndMenu();
						}

						if (ImGui::MenuItem("Diffuse")) newEffectName = "default";
						if (ImGui::MenuItem("Diffuse, Spec")) newEffectName = "default_spec";
						if (ImGui::MenuItem("Diffuse, Normal")) newEffectName = "normal";
						if (ImGui::MenuItem("Diffuse, Normal, Spec")) newEffectName = "normal_spec";

						if (ImGui::MenuItem("Emissive")) newEffectName = "emissive";
						if (ImGui::MenuItem("Emissive Additive Alpha")) newEffectName = "emissive_additive_alpha";
						if (ImGui::MenuItem("Emissive Night")) newEffectName = "emissivenight";
						if (ImGui::MenuItem("Emissive Strong")) newEffectName = "emissivestrong";

						ImGui::EndMenu();
					}
					if (ImGui::BeginMenu("Vehicle"))
					{
						if (ImGui::BeginMenu("Paint"))
						{
							ImGui::EndMenu();
						}
						ImGui::EndMenu();
					}
					if (ImGui::BeginMenu("Ped"))
					{
						ImGui::EndMenu();
					}

					if (newEffectName)
					{
						// TODO: Binary search...
						rage::grcEffect* newEffect = nullptr;
						for (int i = 0; i < 512; i++)
						{
							if (g_Effects[i] == nullptr)
								break;

							if (String::Equals(g_Effects[i]->GetName(), newEffectName))
							{
								newEffect = g_Effects[i];
								break;
							}
						}

						if (newEffect)
						{
							shader->SetEffect(newEffect);
						}
					}
					ImGui::EndPopup();
				}
				ImGui::PopStyleVar(2); // WindowPadding, ItemSpacing
			}

			SlGui::PushFont(SlFont_Medium);
			SlGui::CategoryText("Properties");
			SlGui::PopFont();

			ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(6, 2));
			bool propertiesTable = ImGui::BeginTable("MaterialEditor_Properties", 2, ImGuiTableFlags_SizingFixedFit);
			ImGui::PopStyleVar(); // ItemInnerSpacing
			if (!propertiesTable)
				return;

			ImGui::TableSetupColumn("MaterialEditor_Properties_Name", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn("MaterialEditor_Properties_Value", ImGuiTableColumnFlags_WidthStretch);

			rage::grcEffect* effect = shader->GetEffect();
			for (u16 i = 0; i < shader->GetVarCount(); i++)
			{
				ImGui::TableNextRow();

				// Material (shader in rage terms) holds variable values without any metadata,
				// we have to query effect variable to get name and type
				rage::grcInstanceVar* var = shader->GetVarByIndex(i);
				rage::grcEffectVar* varInfo = effect->GetVarByIndex(i);

				// Column: Variable Name
				if (ImGui::TableNextColumn())
				{
					ImGui::Text("%s", varInfo->GetName());
				}

				// Column: Variable value
				if (ImGui::TableNextColumn())
				{
					ConstString inputID = ImGui::FormatTemp("###%s_%i", varInfo->GetName(), i);

					// Stretch input field
					ImGui::SetNextItemWidth(-1);

					switch (varInfo->GetType())
					{
					case rage::EFFECT_VALUE_INT:
					case rage::EFFECT_VALUE_INT1:		ImGui::InputInt(inputID, var->GetValuePtr<int>());		break;
					case rage::EFFECT_VALUE_INT2:		ImGui::InputInt2(inputID, var->GetValuePtr<int>());		break;
					case rage::EFFECT_VALUE_INT3:		ImGui::InputInt3(inputID, var->GetValuePtr<int>());		break;
					case rage::EFFECT_VALUE_FLOAT:		ImGui::InputFloat(inputID, var->GetValuePtr<float>());	break;
					case rage::EFFECT_VALUE_VECTOR2:	ImGui::InputFloat2(inputID, var->GetValuePtr<float>());	break;
					case rage::EFFECT_VALUE_VECTOR3:	ImGui::InputFloat3(inputID, var->GetValuePtr<float>());	break;
					case rage::EFFECT_VALUE_VECTOR4:	ImGui::InputFloat4(inputID, var->GetValuePtr<float>());	break;
					case rage::EFFECT_VALUE_BOOL:		SlGui::Checkbox(inputID, var->GetValuePtr<bool>());		break;

					case rage::EFFECT_VALUE_TEXTURE:
					{
						rage::grcTexture* newTexture = DrawTexturePicker(inputID, var->GetValuePtr<rage::grcTexture>());
						if (newTexture)
							var->SetTexture(newTexture);
					}
					break;

					// Unsupported types for now:
					// - EFFECT_VALUE_MATRIX34
					// - EFFECT_VALUE_MATRIX44
					// - EFFECT_VALUE_STRING
					default:
						ImGui::Dummy(ImVec2(0, 0));
						break;
					}
				}
			}
			ImGui::EndTable(); // MaterialEditor_Properties
			SlGui::EndPadded(); // MaterialEditor_Properties_Padding
		}

		void MaterialEditor()
		{
			if (!SlGui::Begin("Material Editor"))
			{
				SlGui::End();
				return;
			}

			if (SlGui::BeginTable("MaterialEditor_SplitView", 2, ImGuiTableFlags_Resizable))
			{
				float defaultListWidth = ImGui::GetTextLineHeight() * 10;
				ImGui::TableSetupColumn("MaterialEditor_ListAndPreview", ImGuiTableColumnFlags_WidthFixed, defaultListWidth);
				ImGui::TableSetupColumn("MaterialEditor_Properties", ImGuiTableColumnFlags_WidthStretch, 0);

				rage::grmShaderGroup* shaderGroup = m_Drawable->GetShaderGroup();

				// Column: Preview & Mat List
				if (ImGui::TableNextColumn())
				{
					DrawMaterialList(shaderGroup);
				}

				// Column: Mat Properties
				if (ImGui::TableNextColumn())
				{
					DrawMaterialProperties(shaderGroup);
				}

				SlGui::EndTable(); // MaterialEditor_SplitView
			}

			SlGui::End();
		}
#endif

		static inline std::atomic_bool bRenderCollision = false;
		static inline std::atomic_bool bRenderedThisFrame = false;

		static inline void(*gImpl_LodGroupDraw)(u64, u64, u64, int, u32);
		static void aImpl_LodGroupDraw(u64 a1, u64 a2, u64 a3, int a4, u32 a5)
		{
			gImpl_LodGroupDraw(a1, a2, a3, a4, a5);

			using namespace rage;

			if (!bRenderCollision)
				return;

			//if (!(a4 & 0x1)) // First bucket
			//	return;

			//if (bRenderedThisFrame)
			//	return;
			//bRenderedThisFrame = true;

			//static BoundRender boundRender;
			//static bool init = false;
			//if (!init)
			//{
			//	boundRender.Init();
			//	init = true;
			//}
		}

		rage::rmcDrawable* sceneDrawable = nullptr;
		void OnRender() override
		{
			//VsHook();

			//{
			//	using namespace asset;

			//	amPtr<TxdAsset> txd = AssetFactory::LoadFromPath<TxdAsset>(L"smh.itd");

			//	Textures& textures = txd->GetTextures();
			//	// Things...

			//	txd->CompileCallback = [](ConstWString message, double percent)
			//		{
			//			// Do smh...
			//		}; 

			//	txd->CompileToFile();
			//}

#ifndef AM_STANDALONE
			static gmAddress findEffectFn = gmAddress::Scan("48 89 5C 24 08 4C 63 1D ?? ?? ?? ?? 48");
			static rage::grcEffect** effects = findEffectFn.GetRef(0xC + 0x3).To<rage::grcEffect**>();


#endif

			using namespace rage;
#ifndef AM_STANDALONE

			static char drawableName[64] = TEST_MODEL_NAME;
			strLocalIndex slot;
			fwAssetStore<rmcDrawable, fwAssetDef<rmcDrawable>>* module = (decltype(module))hooks::Streaming::GetModule("ydr");
			module->GetSlotIndexByName(slot, drawableName);
			gtaDrawable* drawable = static_cast<gtaDrawable*>(module->GetPtr(slot));

			//m_Drawable = drawable;

			if (m_Drawable)
			{
				MaterialEditor();
			}
#endif
			//return;
			//ImPlot::ShowDemoWindow();
			//ImGui::ShowMetricsWindow();
			//ImGui::SetNextWindowSize(ImVec2(512, 512), ImGuiCond_Always);
			ImGui::Begin("RageAm Testbed");

			if (ImGui::Button("YDR"))
			{
				gtaDrawable* drawable;
				rage::pgRscBuilder::Load(&drawable, "C:/Users/falco/Desktop/cable1_root.ydr", 165);

				if (drawable->HasMap())
					GetAllocator(ALLOC_TYPE_VIRTUAL)->Free(drawable);
				else
					delete drawable;

				AM_TRACEF("");
			}

			using namespace graphics;

			// TODO: Better scene debugger

			static amPtr<Scene> myScene;
			if (ImGui::Button("Skinning"))
			{
				amPtr<Scene> skinScene = SceneFactory::LoadFrom(L"C:/Users/falco/Desktop/collider.idr/mesh.gltf");

				//SceneData blendIndices;
				//skinScene->GetNode(1)->GetMesh()->GetGeometry(0)->GetAttribute(blendIndices, BLENDINDICES, 0);

				//SceneData blendWeights;
				//skinScene->GetNode(1)->GetMesh()->GetGeometry(0)->GetAttribute(blendWeights, BLENDWEIGHT, 0);

				//struct BlendIndices { u8 x, y, z, w; };
				//BlendIndices* bi = blendIndices.GetBufferAs<BlendIndices>();

				SceneNode* skinnedNode = skinScene->GetNode(1);
				//DirectX::XMMATRIX inverseBind = skinnedNode->GetInverseBoneMatrix(0);
				//DirectX::XMMATRIX nodeForm = skinnedNode->GetWorldTransform();

				//DirectX::XMMATRIX boneForm = XMMatrixMultiply(nodeForm, inverseBind);

				DirectX::XMMATRIX boneMatrix = skinnedNode->GetBone(1)->GetLocalTransform();
				/*DirectX::XMMATRIX yzSwap = DirectX::XMMatrixSet
				(
					1, 0, 0, 0,
					0, 0, 1, 0,
					0, -1, 0, 0,
					0, 0, 0, 1
				);*/
				/*static const auto yzSwap = DirectX::XMMatrixRotationAxis(VEC_X, Math::DegToRad(-90));
				boneMatrix = XMMatrixMultiply(yzSwap, boneMatrix);*/
				boneMatrix.r[0] = rage::Vec3V(boneMatrix.r[0]).XZY();
				boneMatrix.r[1] = rage::Vec3V(boneMatrix.r[1]).XZY();
				boneMatrix.r[2] = rage::Vec3V(boneMatrix.r[2]).XZY();
				boneMatrix.r[3] = rage::Vec3V(boneMatrix.r[3]).XZY();

				/*		DirectX::XMVECTOR scale, rot, trans;
						XMMatrixDecompose(&scale, &rot, &trans, boneMatrix);*/
				rage::QuatV			m_DefaultRotation;
				rage::Vec3V			m_DefaultTranslation;
				rage::Vec3V			m_DefaultScale;
				rage::Mat44V(boneMatrix).Decompose(&m_DefaultTranslation, &m_DefaultScale, &m_DefaultRotation);
				//skinnedNode->GetWorldBoneTransform(1);

				AM_TRACEF("");
			}

			if (ImGui::Button("ITD"))
			{
				using namespace asset;

				TxdAssetPtr txd = AssetFactory::LoadFromPath<TxdAsset>(L"C:/Users/falco/Desktop/collider.idr/textures.itd");
				txd->CompileToFile(L"C:/Users/falco/Desktop/textures.ytd");
			}

			if (myScene)
			{
				if (myScene->GetNodeCount() != 1)
					ImGui::Text("Scene has no root node");
				else
					ImGui::Text("Scene has root node");

				// Print node, all siblings and it's children
				std::function<void(SceneNode*)> recurseNode;
				recurseNode = [&](const SceneNode* node)
					{
						ImGui::Indent();
						while (node)
						{
							ImGui::BulletText(node->GetName());

							ImGui::SameLine();
							auto next = node->GetNextSibling();
							ImGui::Text("Next: %s", next ? next->GetName() : "-");

							ImGui::SameLine();
							auto child = node->GetFirstChild();
							ImGui::Text("Child: %s", child ? child->GetName() : "-");

							if (!node->HasMesh())
							{
								ImGui::SameLine();
								ImGui::TextColored(ImVec4(1, 0, 0, 1), "No Mesh");
							}

							// Print all children
							recurseNode(node->GetFirstChild());

							node = node->GetNextSibling();
						}
						ImGui::Unindent();
					};
				recurseNode(myScene->GetFirstNode());
			}




			//#ifndef AM_STANDALONE
			//
			//
			//
			//			bool rc = bRenderCollision;
			//			ImGui::Checkbox("Render Collision", &rc);
			//			bRenderCollision = rc;
			//			static bool isInit = false;
			//			if (!isInit)
			//			{
			//				isInit = true;
			//
			//				// changed to build primitive draw list
			//				static gmAddress lodGroupDraw = gmAddress::Scan("48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 41 54 41 55 41 56 41 57 48 83 EC 30 48 63 B4");
			//
			//				Hook::Create(lodGroupDraw, aImpl_LodGroupDraw, &gImpl_LodGroupDraw, true);
			//			}
			//			bRenderedThisFrame = false;
			//#endif

#ifndef AM_STANDALONE

			auto ymapStore = hooks::Streaming::GetModule("ymap");
			strLocalIndex ymapSlot;
			ymapStore->GetSlotIndexByName(ymapSlot, "ss1_09_long_0");

			ImGui::Text("YMAP SLOT: %u", ymapSlot);

			if (ymapSlot != INVALID_STR_INDEX)
			{
				ImGui::Text("YMAP PTR: %p", ymapStore->GetPtr(ymapSlot));
			}
#endif

#define TESTBED_DIR L"C:/Users/falco/Desktop/RageAm TestBed/"

			using namespace asset;

			if (ImGui::Button("Compile Monkey"))
			{
				amPtr<DrawableAsset> drAsset = AssetFactory::LoadFromPath<DrawableAsset>(
					L"C:/Users/falco/Desktop/monkey.idr");
				drAsset->CompileToFile();
			}

			if (ImGui::Button("Load & Delete Drawable"))
			{

				amPtr<DrawableAsset> drAsset = AssetFactory::LoadFromPath<DrawableAsset>(
					L"C:/Users/falco/Desktop/monkey.idr");

				gtaDrawable* dr = new gtaDrawable();
				drAsset->CompileToGame(dr);
				delete dr;
			}

			if (ImGui::Button("Prologue Cull"))
			{
				struct CIplCullBoxEntry
				{
					atHashValue		Name;
					spdAABB			BoundingBox;
					char			pad[16];
					atArray<u32>	CulledContainerHashes;
					bool			bEnabled;
				};

				auto& cullBoxes = *gmAddress::Scan("48 8B 05 ?? ?? ?? ?? 39 08 74 0E").GetRef(3).To<atArray<CIplCullBoxEntry>*>();
				for (CIplCullBoxEntry& entry : cullBoxes)
				{
					if (entry.Name == joaat("prologue_Main"))
					{
						entry.BoundingBox.Min = { -100, -100, -100 };
						entry.BoundingBox.Max = { 100, 100, 100 };
						break;
					}
				}
			}

			if (ImGui::Button("Load Drawable Asset"))
			{

				amPtr<DrawableAsset> drAsset = AssetFactory::LoadFromPath<DrawableAsset>(
					L"C:/Users/falco/Desktop/monkey.idr");

#ifndef AM_STANDALONE
				gtaDrawable* dr = new gtaDrawable();
				drAsset->CompileToGame(dr);

				sceneDrawable = dr;
				m_Drawable = dr;

				// gtaDrawable
				//integration::VftPatch::Install(sceneDrawable, "48 8D 05 ?? ?? ?? ?? 33 FF 48 89 03 48 8B 93 B0 00 00 00");
#endif

				AM_TRACEF("");
		}

#ifdef AM_STANDALONE
			if (ImGui::Button("Import Textured Cube"))
			{
				auto scene = graphics::SceneFactory::LoadFrom(L"C:/Users/falco/Desktop/textcube.idr/mesh.gltf");

				AM_TRACEF("");
			}
#endif
			if (ImGui::Button("Composite Drawable"))
			{
				gtaDrawable* drawable;
				pgRscBuilder::Load(&drawable, "C:/Users/falco/Desktop/RageAm TestBed/prop_metal_plates02.ydr", 165);

				auto comp = drawable->GetBound();

				AM_TRACEF("");
			}

			if (ImGui::Button("Drawable w Skeleton"))
			{
				gtaDrawable* drawable;
				pgRscBuilder::Load(&drawable, "C:/Users/falco/Desktop/RageAm TestBed/Skeleton/des_server_root.ydr", 165);

				crSkeletonData* skeletonData = drawable->GetSkeletonData();
				//crBoneData& boneData = skeletonData->m_Bones[10];

				AM_TRACEF("");
			}

			if (ImGui::Button("IDR"))
			{
				using namespace asset;

				amPtr<DrawableAsset> drAsset = AssetFactory::LoadFromPath<DrawableAsset>(L"C:/Users/falco/Desktop/cubic.idr");

				AM_TRACEF("");
			}

			using namespace graphics;

			auto makeSkeleton = [](const amPtr<Scene>& scene) -> crSkeletonData*
				{
					//bool hasTransform = scene->HasTransform();
					//if (!hasTransform)
					//	return nullptr;

					u16 boneCount = scene->GetNodeCount();
					boneCount += 1; // First bone has to be root

					crSkeletonData* skeletonData = new crSkeletonData();
					skeletonData->Init(boneCount);

					// Setup root bone
					skeletonData->GetBone(0)->SetName(scene->GetName());

					// Setup bone for every model
					for (u16 i = 1, k = 0; i < boneCount; i++, k++)
					{
						crBoneData* bone = skeletonData->GetBone(i);
						SceneNode* node = scene->GetNode(k);

						bone->SetIndex(i);
						bone->SetParentIndex(0); // TODO: For now... We can use index from cgltf hierarchy
						if (i + 1 < boneCount)
							bone->SetNextIndex(i + 1);

						bone->SetName(node->GetName());
						// TODO: Test
						Vec3V trans = node->GetTranslation();
						bone->SetTransform(&trans, nullptr, nullptr);
					}
					skeletonData->Finalize();
					return skeletonData;
				};

			if (ImGui::Button("GLTF Anim"))
			{
				amPtr<Scene> scene = SceneFactory::LoadFrom(TESTBED_DIR L"anim.gltf");
				crSkeletonData* skeleton = makeSkeleton(scene);
			}

			// integration::ScriptHookThread* shvThread = integration::ScriptHook::GetThread();

			static constexpr const char* RAGEAM = "RAGEAM";
			static constexpr u32 RAGEAM_HASH = joaat(RAGEAM);
			static strLocalIndex RAGEAM_SLOT = INVALID_STR_INDEX;
			static strLocalIndex RAGEAM_BOUND_SLOT = INVALID_STR_INDEX;

#ifndef AM_STANDALONE
			static int handle = 0;
			if (ImGui::Button("Spawn Entity"))
			{
				//static gmAddress addr = gmAddress::Scan("48 8B C4 48 89 58 08 48 89 70 10 48 89 78 18 4C 89 60 20 55 41 55 41 56 48 8D 68 D9");
				//void(*spawnEntity)(u32 hash, float x, float y, float z, bool a5, bool a6, int a7, u32 * handle, bool, bool, bool) =
				//	addr.To<decltype(spawnEntity)>();

				//spawnEntity(
				//	joaat("prop_metal_plates02"),
				//	-690.0f, 218.46f, 141.0f, true, true, -1, &handle, false, false, false);

				handle = SHV::OBJECT::CREATE_OBJECT_NO_OFFSET(RAGEAM_HASH, -677, 168, 73, FALSE, TRUE, FALSE);
				/*shvThread->Invoke([]
					{
						handle = SHV::OBJECT::CREATE_OBJECT(joaat("prop_metal_plates02"), -690.0f, 218.46f, 141.0f, FALSE, TRUE, FALSE);
					});*/
			}

			if (ImGui::Button("Spawn Test Entity"))
			{
				handle = SHV::OBJECT::CREATE_OBJECT_NO_OFFSET(joaat("prop_metal_plates02"), -677, 168, 73, FALSE, TRUE, FALSE);
			}

			if (ImGui::Button("Delete Entity") && handle != 0)
			{
				//shvThread->Invoke([]
				//	{
				//		//SHV::ENTITY::SET_ENTITY_AS_MISSION_ENTITY(handle, FALSE, TRUE);
				//		//SHV::ENTITY::DELETE_ENTITY(&handle);
				//	});

				//SHV::ENTITY::SET_ENTITY_AS_MISSION_ENTITY(handle, FALSE, TRUE);
				//SHV::ENTITY::DELETE_ENTITY(&handle);
				SHV::ENTITY::SET_ENTITY_COORDS_NO_OFFSET(handle, 1900, 1900, 900, true, true, true);
				SHV::ENTITY::SET_ENTITY_AS_NO_LONGER_NEEDED(&handle);
				handle = 0;

				//static gmAddress getEntityFromGUID_Addr = gmAddress::Scan("E8 ?? ?? ?? ?? 33 FF 4C 8B 03").GetCall();
				//u64(*getEntityFromGUID)(u32 h) = getEntityFromGUID_Addr.To<decltype(getEntityFromGUID)>();

				//u64 entity = getEntityFromGUID(handle);

				////static gmAddress deleteEntityObject_Addr = gmAddress::Scan("48 85 C9 74 31 53");
				////void(*deleteEntityObject)(u64 e) = deleteEntityObject_Addr.To<decltype(deleteEntityObject)>();

				////deleteEntityObject(entity);

				//void (*dctor)(u64, bool del) = (decltype(dctor))**(u64**)entity;
				//dctor(entity, true);

				//handle = 0;
			}
			ImGui::Text("HANDLE: %u", handle);


			if (ImGui::Button("Register Archetype"))
			{
				//static gmAddress initArchetypeFromDef_Addr = gmAddress::Scan("89 54 24 10 53 56 57 48 83 EC 20 48");
				// replaced fwArchetype to CBaseModelInfo
				static gmAddress initArchetypeFromDef_Addr = gmAddress::Scan("48 89 5C 24 08 57 48 83 EC 20 49 8B F8 48 8B D9 E8 ?? ?? ?? ?? 44");
				static void(__fastcall * initArchetypeFromDef)(CBaseModelInfo*, strLocalIndex, fwArchetypeDef*, bool) =
					initArchetypeFromDef_Addr.To<decltype(initArchetypeFromDef)>();

				static gmAddress baseModelInfoCtor_Addr = gmAddress::Scan("65 48 8B 14 25 58 00 00 00 48 8D 05 ?? ?? ?? ?? 45");
				static void(__fastcall * baseModelInfoCtor)(CBaseModelInfo*) =
					baseModelInfoCtor_Addr.To<decltype(baseModelInfoCtor)>();

				CBaseModelInfo* modelInfo = (CBaseModelInfo*)rage_malloc(0xB0);
				baseModelInfoCtor(modelInfo);

				auto& drBb = m_Drawable->GetLodGroup().GetBoundingBox();
				auto& drBs = m_Drawable->GetLodGroup().GetBoundingSphere();

				CBaseArchetypeDef modelDef{};
				modelDef.m_Name = joaat("rageam");/*RAGEAM_HASH*/
				modelDef.m_AssetType = fwArchetypeDef::ASSET_TYPE_DRAWABLE;
				modelDef.m_BoundingBox = spdAABB::Infinite(); // drBb;
				modelDef.m_BsCentre = drBs.GetCenter();
				modelDef.m_BsRadius = 100000;//drBs.GetRadius().Get();
				modelDef.m_AssetName = RAGEAM_HASH;
				modelDef.m_LodDist = 100000;
				modelDef.m_PhysicsDictionary = RAGEAM_HASH; // TODO: Disable for now...
				modelDef.m_Flags = ADF_STATIC | ADF_BONE_ANIMS;
				//modelDef.m_TextureDictionary = rage::joaat("adder");

				initArchetypeFromDef(modelInfo, 0, &modelDef, /*false*/ true);

				modelInfo->m_Flags |= 1; // Drawable loaded?

				// TODO: Must be done by init archetype...
				static gmAddress registerArchetype_Addr = gmAddress::Scan("48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 20 48 83 3D ?? ?? ?? ?? ?? 8B FA");
				static void(__fastcall * registerArchetype)(CBaseModelInfo*, strLocalIndex) =
					registerArchetype_Addr.To<decltype(registerArchetype)>();
				registerArchetype(modelInfo, 0);
			}

			if (0)
			{
				//  render all entity bounds from pool

				//using namespace rage;
				//static rage::fwBasePool* buildingPool = *gmAddress::Scan("48 8B 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 85 C0 74 10 BA 0D 00 00 00").GetRef(3).To<fwBasePool**>();
				//for (s32 i = 0; i < buildingPool->GetSize(); i++)
				//{
				//	char* entity = (char*)buildingPool->GetSlot(i);
				//	if (!entity)
				//		continue;

				//	Mat44V mtx;
				//	mtx.r[0] = *(Vec3V*)(entity + 0x60);
				//	mtx.r[1] = *(Vec3V*)(entity + 0x70);
				//	mtx.r[2] = *(Vec3V*)(entity + 0x80);
				//	mtx.r[3] = *(Vec3V*)(entity + 0x90);

				//	static gmAddress addr = gmAddress::Scan("E8 ?? ?? ?? ?? 48 8B D5 0F 28 D6").GetCall();
				//	static phInst* (*getPhysics)(u64) = addr.To<decltype(getPhysics)>();

				//	phInst* phInst = getPhysics((u64)entity);
				//	if (!phInst)
				//		continue;

				//	phArchetype* arch = phInst->GetArchetype();
				//	if (!arch)
				//		continue;

				//	phBound* bound = arch->GetBound();
				//	if (!bound)
				//		continue;

				//	boundRender.Render(bound, mtx);
				//}
			}


			if (ImGui::Button("Shutdown LS"))
			{
				gmAddress addr = gmAddress::Scan("48 89 5C 24 08 56 48 83 EC 20 8B 05");
				void(*shutdown)() = addr.To<decltype(shutdown)>();
				shutdown();
			}

			ImGui::Text("SIZE :%u", sizeof fwArchetype);
			ImGui::Text("OFFSET :%u", offsetof(fwArchetype, m_BsCentre));

			fwAssetStore<rmcDrawable, fwAssetDef<rmcDrawable>>* dr = (decltype(dr))hooks::Streaming::GetModule("ydr");
			fwAssetStore<phBound, fwAssetDef<phBound>>* bn = (decltype(bn))hooks::Streaming::GetModule("ybn");

			if (ImGui::Button("Inject Drawable"))
			{
				strLocalIndex st = -1;
				u32 hash = RAGEAM_HASH;
				dr->AddSlot(st, hash);
				dr->Set(st, sceneDrawable);

				RAGEAM_SLOT = st;
			}

			if (ImGui::Button("Inject Collision"))
			{
				strLocalIndex st = -1;
				u32 hash = RAGEAM_HASH;
				bn->AddSlot(st, hash);
				bn->Set(st, ((gtaDrawable*)m_Drawable)->GetBound());

				RAGEAM_BOUND_SLOT = st;
			}

			if (RAGEAM_SLOT != INVALID_STR_INDEX)
				ImGui::Text("DW STORE DEF PTR: %p", dr->GetDef(RAGEAM_SLOT));
			ImGui::Text("DW STORE SLOT: %u", RAGEAM_SLOT);
			ImGui::Text("BN STORE SLOT: %u", RAGEAM_BOUND_SLOT);

			//gtaDrawable* gta = (gtaDrawable*)(m_Drawable);
			//if (ImGui::Button("Add Light"))
			//{
			//	CLightAttr light{};
			//	gta->m_Lights.Add(light);
			//}

			//if (gta->m_Lights.GetSize() > 0)
			//{
			//	CLightAttr& light = gta->m_Lights.Get(0);
			//	light.FixupVft();
			//	light.Type = 1; // Point
			//	light.Position = { 0,0,1 };
			//	light.ColorR = 255;
			//	light.ColorG = 255;
			//	light.ColorB = 255;
			//	light.Direction = { 0, -0.9063082, -0.4226171 };
			//	light.Tangent = { -1, 0, 0 };
			//	light.Intensity = 25;
			//	light.Flags = 65536;
			//	light.TimeFlags = 16777215;
			//	light.Fallof = 2.5;
			//	light.FallofExponent = 32;
			//	light.ConeInnerAngle = 8;
			//	light.ConeOuterAngle = 50;
			//	light.Extent = { 1, 1 ,1 };
			//	light.CoronaZBias = 0.1;
			//	light.ProjectedTexture = 1337081687;
			//	light.VolumeOuterColorR = 255;
			//	light.VolumeOuterColorG = 255;
			//	light.VolumeOuterColorB = 255;
			//	light.VolumeOuterExponent = 1;
			//	light.ShadowNearClip = 0.01;
			//	light.CullingPlaneNormal = {
			//	 0.08755547, -0.5555016, -0.826893 };
			//	light.CullingPlaneOffset = 2.5;
			//}
#endif

#ifndef AM_STANDALONE
			static gmAddress isInCd_Addr = gmAddress::Scan("48 83 EC 28 8B 44 24 38 48 8D 54 24 40 C7 44 24 40 FF FF 00 00 0D FF FF FF 0F 25 FF FF FF 0F 89 44 24 38 E8 ?? ?? ?? ?? 0F B7 44 24 40 66 89 44 24 38 8B 44 24 38 0D 00 00 FF 0F 0F BA F0 1C 89");
			static bool(*isInCd)(u32 hash) = isInCd_Addr.To<decltype(isInCd)>();

			ImGui::Text("adder CD: %s", isInCd(joaat("adder")) ? "True" : "False");
			ImGui::Text("rageam CD: %s", isInCd(joaat("rageam")) ? "True" : "False");
#endif

			if (ImGui::Button("Static Bound"))
			{
				phBound* bound;
				pgRscBuilder::Load(&bound, "C://Users//falco//Desktop//RageAm TestBed/ch3_01_5.ybn", 43);

				AM_TRACE("");
			}

			//ImGui::InputScalar("Entity Address", ImGuiDataType_U64, &entity);
#ifdef PHTEST
			if (ImGui::Button("phInst"))
			{
				static gmAddress addr = gmAddress::Scan("E8 ?? ?? ?? ?? 48 8B D5 0F 28 D6").GetCall();
				static phInst* (*getPh)(u64) = addr.To<decltype(getPh)>();

				phInst* phInst = getPh(entity);
				//((rage::phInst * (*)(u64))(*(u64*)entity + 0x60))(entity);
				//phBound* bound = new phBoundBox(m_Drawable->GetLodGroup().GetBoundingBox());
				phBound* bound = ((gtaDrawable*)m_Drawable)->GetBound();
				phArchetypePhys* arch = new phArchetypePhys();
				arch->SetBound(bound);
				//bound->m_MaterialID0 = 56;
				//phInst->SetArchetype(arch);
				phInst->m_Archetype.SetNoRef(arch);
				//((gtaDrawable*)m_Drawable)->SetBound(bound);
			}
#endif

			if (ImGui::Button("MTX"))
			{
				DirectX::XMMATRIX mtx = DirectX::XMMatrixIdentity();
				mtx.r[0].m128_f32[0] = 0.00439453125;
				mtx.r[0].m128_f32[1] = 0.4033203;
				mtx.r[0].m128_f32[2] = 0.8149414;
				mtx.r[1].m128_f32[1] = -0.8066406;
				mtx.r[2].m128_f32[2] = -1.62988281;

				DirectX::XMVECTOR p1 = DirectX::XMVectorSet(1, 0, 0, 0);
				DirectX::XMVECTOR p2 = DirectX::XMVectorSet(-1, 0, 0, 0);
				DirectX::XMVECTOR p3 = DirectX::XMVectorSet(0, 0, 0, 0);

				DirectX::XMVECTOR p1t = DirectX::XMVector3Transform(p1, mtx);
				DirectX::XMVECTOR p2t = DirectX::XMVector3Transform(p2, mtx);
				DirectX::XMVECTOR p3t = DirectX::XMVector3Transform(p3, mtx);

				AM_TRACEF("");
			}

			static int matid = 56;
			if (ImGui::InputInt("MaterialID0", &matid))
			{
				//((gtaDrawable*)m_Drawable)->GetBound()->m_MaterialID0 = matid;
			}

			//
			//#if AM_STANDALONE

			//if (ImGui::Button("Split"))
			//{
			//	using namespace graphics;

			//	grcFvf format{};
			//	format.Channels |= 1 << 0; // POSITION
			//	format.Channels |= 1 << 3; // NORMAL
			//	format.Channels |= 1 << 4; // COLOR0
			//	format.Channels |= 1 << 6; // TEXCOORD0
			//	format.Stride = sizeof Vector3 + sizeof Vector3 + sizeof u32 + sizeof Vector2;
			//	format.ChannelCount = 4;
			//	format.ChannelFormats |= GRC_FORMAT_R32G32B32_FLOAT;
			//	format.ChannelFormats |= GRC_FORMAT_R32G32B32_FLOAT << 12;
			//	format.ChannelFormats |= GRC_FORMAT_R8G8B8A8_UNORM << 16;
			//	format.ChannelFormats |= GRC_FORMAT_R32G32_FLOAT << 24;
			//	grcVertexDeclaration* decl = grcVertexDeclaration::CreateFromFvf(format);

			//	amPtr<Scene> scene = SceneFactory::LoadFrom(L"C:/Users/falco/Desktop/ball.gltf");
			//	for (u32 i = 0; i < scene->GetModelCount(); i++)
			//	{
			//		SceneModel* sceneModel = scene->GetModel(i);
			//		for (u32 k = 0; k < sceneModel->GetGeometriesCount(); k++)
			//		{
			//			SceneGeometry* sceneGeometry = sceneModel->GetGeometry(k);

			//			// Set Vertex & Index data
			//			u32 vtxCount = sceneGeometry->GetVertexCount();
			//			u32 idxCount = sceneGeometry->GetIndexCount();
			//			VertexBufferEditor vtxBufferEditor(decl, vtxCount);
			//			vtxBufferEditor.SetFromGeometry(sceneGeometry);
			//			SceneData indices;
			//			sceneGeometry->GetIndices(indices);

			//			// If indices are 16 bit we can just use them as is, otherwise we'll have to split geometry
			//			if (indices.Format == DXGI_FORMAT_R16_UINT)
			//			{
			//				AM_TRACEF("");
			//			}
			//			else
			//			{
			//				AM_ASSERT(indices.Format == DXGI_FORMAT_R32_UINT, "Unsupported index buffer format %s", Enum::GetName(indices.Format));
			//				auto splitVertices = MeshSplitter::Split(vtxBufferEditor.GetBuffer(), decl->Stride, (u32*)indices.Buffer, idxCount);
			//				for (const MeshChunk& chunk : splitVertices)
			//				{
			//					AM_TRACEF("");
			//				}
			//			}
			//		}
			//	}
			//}

			if (ImGui::Button("Bound"))
			{
				phBoundBox box(Vector3(23.0f, 12.0f, 55.0f));



				phBoundComposite* composite = new phBoundComposite();
				composite->Init(1, false);
				composite->SetBound(0, &box);
				composite->SetTypeFlags(0,
					CF_MAP_WEAPON | CF_MAP_DYNAMIC | CF_MAP_ANIMAL | CF_MAP_COVER | CF_MAP_VEHICLE);
				composite->SetIncludeFlags(0,
					CF_VEHICLE_NOT_BVH | CF_VEHICLE_BVH | CF_PED | CF_RAGDOLL | CF_ANIMAL | CF_ANIMAL_RAGDOLL | CF_OBJECT | CF_PLANT | CF_PROJECTILE | CF_EXPLOSION | CF_FORKLIFT_FORKS | CF_TEST_WEAPON | CF_TEST_CAMERA | CF_TEST_AI | CF_TEST_SCRIPT | CF_TEST_VEHICLE_WHEEL | CF_GLASS);

				//phBoundBox box(Vector3(5, 1, 1));

				//static gmAddress addr = gmAddress::Scan("48 89 5C 24 08 57 48 83 EC 20 48 8B DA 48 8B F9 E8 ?? ?? ?? ?? 48 C7");
				//void(*phBoundBox_ctor)(phBoundBox*, Vec3V*) = addr.To<decltype(phBoundBox_ctor)>();

				//Vec3V size = Vec3V(23.0f, 12.0f, 55.0f);
				//phBoundBox boxGame(Vec3V(0.0f));
				//phBoundBox_ctor(&boxGame, &size);

				AM_TRACEF("");
			}

			// ImGui::Text("Bound: %p", ((gtaDrawable*)m_Drawable)->GetBound());

#ifndef AM_STANDALONE
			if (ImGui::Button("Dump Shaders"))
			{
				XmlDoc doc("Effects");

				XmlHandle xShaders = doc.Root();

				for (int i = 0; i < 512; i++)
				{
					if (effects[i] == nullptr)
						break;

					grcEffect* effect = effects[i];

					// Get vertex input declaration for default draw technique
					grcEffectTechnique* draw = effect->GetTechnique("draw");
					if (!draw)
						continue; // Shader is not supported
					grcVertexProgram* vs = effect->GetVS(draw);
					if (!vs->HasByteCode())
						continue;
					grcVertexDeclaration* decl;
					vs->ReflectVertexFormat(&decl);

					// Serialize parameters
					XmlHandle xeffect = xShaders.AddChild(effect->GetName());
					xeffect.SetAttribute("VertexStride", decl->Stride);
					for (u32 k = 0; k < decl->ElementCount; k++)
					{
						grcVertexElementDesc& element = decl->Elements[k];
						XmlHandle xElement = xeffect.AddChild(element.SemanticName);
						xElement.SetAttribute("SemanticIndex", element.SemanticIndex);
						xElement.SetAttribute("Format", Enum::GetName(element.Format));
						xElement.SetAttribute("Offset", decl->GetElementOffset(k));
			}
	}
				doc.SaveToFile(L"C:/Users/falco/Desktop/Effects.xml");
			}
#endif


#ifdef NOTHINGGG
			if (ImGui::Button("New Scene"))
			{
#ifndef AM_STANDALONE
				using namespace graphics;

				grcEffect* defaultEffect;

				u32 effectName = joaat("normal_spec");
				for (int i = 0; i < 512; i++)
				{
					if (effects[i]->GetNameHash() == effectName)
					{
						defaultEffect = effects[i];
						break;
					}
				}

				grcEffectTechnique* draw = &defaultEffect->m_Techniques[0]; // Draw
				//effect->GetTechniqueByHandle(effect->LookupTechniqueByHashKey(joaat("draw")));
				grcVertexProgram* vs = &defaultEffect->m_VertexPrograms[draw->m_Passes[0].VS];

				grcVertexDeclaration* decl;
				grcFvf fvf;
				vs->ReflectVertexFormat(&decl, &fvf);

				//amPtr<Scene> scene = SceneFactory::LoadFrom(L"C:/Users/falco/Desktop/RageAm TestBed/test.gltf");
				//amPtr<Scene> scene = SceneFactory::LoadFrom(L"C:/Users/falco/Desktop/RageAm TestBed/normals_scene.gltf");
				//amPtr<Scene> scene = SceneFactory::LoadFrom(L"C:/Users/falco/Desktop/RageAm TestBed/monkey_low.gltf"); // suz
				amPtr<Scene> scene = SceneFactory::LoadFrom(L"C:/Users/falco/Desktop/RageAm TestBed/anim2.gltf");

				gtaDrawable* gameDrawable = new gtaDrawable();

				// Skeleton
				{
					gameDrawable->SetSkeletonData(makeSkeleton(scene));
				}

				// Test bound
				//phBoundBox* testBound = new phBoundBox(Vec3V(2, 2, 2));
				{
					amPtr<Scene> col = SceneFactory::LoadFrom(L"C:/Users/falco/Desktop/RageAm TestBed/monkey_low.gltf"); // suz_col

					phBoundGeometry* geometryBound = new phBoundGeometry();

					auto geom = col->GetModel(0)->GetGeometry(0);
					SceneData indices;
					SceneData vertices;
					geom->GetIndices(indices);
					geom->GetAttribute(vertices, POSITION, 0);

					geometryBound->SetMesh(
						geom->GetAABB(),
						(Vector3*)vertices.Buffer, (u16*)indices.Buffer,
						geom->GetVertexCount(), geom->GetIndexCount());

					auto addr = gmAddress::Scan("E8 ?? ?? ?? ?? 48 03 DE").GetCall();
					auto ro = addr.To<void(*)(phBoundGeometry*)>();
					ro(geometryBound);

					phBoundComposite* composite = new phBoundComposite();
					composite->Init(1, false);
					composite->SetBound(0, geometryBound);
					//composite->SetTypeFlags(0, UINT_MAX);
					//composite->SetIncludeFlags(0, UINT_MAX);
					composite->SetTypeFlags(0, 62);
					//CF_MAP_WEAPON | CF_MAP_DYNAMIC | CF_MAP_ANIMAL | CF_MAP_COVER | CF_MAP_VEHICLE | CF_PED);
					composite->SetIncludeFlags(0, 133414592);
					//CF_MAP_WEAPON | CF_MAP_DYNAMIC | CF_MAP_ANIMAL | CF_MAP_COVER | CF_MAP_VEHICLE | CF_PED);

					*(u64*)composite = gmAddress::Scan("48 8d 05 5c 4d 6a 00").GetRef(3);

					// TODO: NO BOUND FOR NOW
					//gameDrawable->SetBound(/*composite*/ geometryBound);

					//geometryBound->WriteObj("C:/Users/falco/Desktop/testbound.obj");
					//geometryBound->WriteObj("C:/Users/falco/Desktop/testbound_shrunk.obj", true);
				}

				// Add materials
				m_MaterialNames.Clear();
				grmShaderGroup* shaderGroup = gameDrawable->GetShaderGroup();
				bool needDefaultMaterial = scene->NeedDefaultMaterial();
				if (needDefaultMaterial)
				{
					grmShader* defaultMaterial = new grmShader(defaultEffect);
					shaderGroup->AddShader(defaultMaterial);
					m_MaterialNames.Construct("Default");
				}
				for (u16 i = 0; i < scene->GetMaterialCount(); i++)
				{
					grmShader* material = new grmShader(defaultEffect);
					shaderGroup->AddShader(material);

					m_MaterialNames.Construct(scene->GetMaterial(i)->GetName());
				}

				rmcLodGroup& lodGroup = gameDrawable->GetLodGroup();
				rmcLod* lod = lodGroup.GetLod(LOD_HIGH);
				for (u32 i = 0; i < scene->GetModelCount(); i++)
				{
					SceneModel* sceneModel = scene->GetModel(i);
					grmModel* gameModel = new grmModel();
					gameModel->SetBoneIndex(i + 1); // TODO: Test
					for (u32 k = 0; k < sceneModel->GetGeometriesCount(); k++)
					{
						SceneGeometry* sceneGeometry = sceneModel->GetGeometry(k);

						// Set Vertex & Index data
						u32 vtxCount = sceneGeometry->GetVertexCount();
						u32 idxCount = sceneGeometry->GetIndexCount();
						VertexBufferEditor vtxBufferEditor(decl, vtxCount);
						vtxBufferEditor.SetFromGeometry(sceneGeometry);
						SceneData indices;
						sceneGeometry->GetIndices(indices);

						// If indices are 16 bit we can just use them as is, otherwise we'll have to split geometry
						if (indices.Format == DXGI_FORMAT_R16_UINT)
						{
							pgUPtr gameGeometry = new grmGeometryQB();
							gameGeometry->SetVertexData(vtxBufferEditor.GetBuffer(), vtxCount, (u16*)indices.Buffer, idxCount, decl, &fvf);
							gameModel->AddGeometry(gameGeometry, sceneGeometry->GetAABB());
						}
						else
						{
							AM_ASSERT(indices.Format == DXGI_FORMAT_R32_UINT, "Unsupported index buffer format %s", Enum::GetName(indices.Format));
							auto splitVertices = MeshSplitter::Split(vtxBufferEditor.GetBuffer(), decl->Stride, (u32*)indices.Buffer, idxCount);
							for (const MeshChunk& chunk : splitVertices)
							{
								pgUPtr gameGeometry = new grmGeometryQB();
								gameGeometry->SetVertexData(chunk.Vertices.get(), chunk.VtxCount, chunk.Indices.get(), chunk.IdxCount, decl, &fvf);
								gameModel->AddGeometry(gameGeometry, sceneGeometry->GetAABB()); // TODO: We have to compute new AABB
							}
						}

						// Setup material
						u16 materialIndex = sceneGeometry->GetMaterialIndex();
						if (needDefaultMaterial) // We have to adjust material indices only if default material is used by some geometry
						{
							if (materialIndex == DEFAULT_MATERIAL)
								materialIndex = 0; // Default material index
							else
								materialIndex += 1; // All scene materials are shifted by one to hold default one at index 0
						}
						gameModel->SetMaterialIndex(k, materialIndex);
					}
					gameModel->ComputeAABB();

					lod->GetModels().Emplace(std::move(gameModel));
				}
				lodGroup.ComputeAABB(LOD_HIGH);

				// Set model
				module->GetDef(slot)->m_pObject = gameDrawable;
				sceneDrawable = gameDrawable;


				// gtaDrawable
				integration::VftPatch::Install(sceneDrawable, "48 8D 05 ?? ?? ?? ?? 33 FF 48 89 03 48 8B 93 B0 00 00 00");
				int i = 0;

#endif
}
#endif

			if (ImGui::Button("Geometry Mesh"))
			{
				using namespace graphics;

				amPtr<Scene> scene = SceneFactory::LoadFrom(L"C:/Users/falco/Desktop/RageAm TestBed/col.gltf");

				phBoundGeometry* testBound = new phBoundGeometry();
				auto geom = scene->GetNode(0)->GetMesh()->GetGeometry(0);
				SceneData indices;
				SceneData vertices;
				geom->GetIndices(indices);
				geom->GetAttribute(vertices, POSITION, 0);
				testBound->SetMesh(
					geom->GetAABB(),
					(Vector3*)vertices.Buffer, (u16*)indices.Buffer,
					geom->GetVertexCount(), geom->GetIndexCount());
				//testBound->WriteObj("C:/Users/falco/Desktop/testbound.obj");
				//testBound->WriteObj("C:/Users/falco/Desktop/testbound_shrunk.obj", true);
			}

			if (ImGui::Button("Drawable"))
			{

#ifdef AM_STANDALONE
				//	phBound t;

				gtaDrawable* drawable;
				rage::pgRscBuilder::Load(
					&drawable, "C:/Users/falco/Desktop/RageAm TestBed/DrawablesWithGeometry/h4_prop_h4_firepit_rocks_01a.ydr", 165);

				phBoundComposite* composite = (phBoundComposite*)drawable->GetBound();
				phBoundGeometry* bg = (phBoundGeometry*)composite->GetBound(0).Get();
				//bg->ComputeNeighbors();
				//bg->SetMarginAndShrink();

				bg->PrintOctantMap();

				bg->WriteObj("C:/Users/falco/Desktop/rocks_game.obj");
				{
					auto indices = bg->GetIndices();
					auto vertices = bg->GetVertices();

					u32 vertexCount = bg->GetVertexCount();
					u32 indexCount = bg->GetIndexCount();

					spdAABB bb;
					//bb.ComputeFrom(vertices.get(), vertexCount);
					bb = bg->GetBoundingBox();

					phBoundGeometry customBound;
					customBound.SetMesh(bb, vertices.get(), indices.get(), vertexCount, indexCount);

					customBound.PrintOctantMap();
					//phBound* bound = &customBound;

					customBound.WriteObj("C:/Users/falco/Desktop/rocks_my.obj");

					// Compare octants...
					bool octantsMatch = true;
					phOctantMap* lMap = bg->GetOctantMap();
					phOctantMap* rMap = customBound.GetOctantMap();
					for (u32 i = 0; i < phOctantMap::MAX_OCTANTS; i++)
					{
						u32 lCount = lMap->IndexCounts[i];
						u32 rCount = rMap->IndexCounts[i];

						if (lCount != rCount)
						{
							AM_ERRF("Octant %u - count mismatch (%u vs %u)", i, lCount, rCount);
							octantsMatch = false;
							break;
						}

						for (u32 k = 0; k < lCount; k++)
						{
							u32 lIdx = lMap->Indices[i][k];
							u32 rIdx = rMap->Indices[i][k];
							if (lIdx != rIdx)
							{
								AM_ERRF("Octant %u - index mismatch at %u (%u vs %u)", i, k, lIdx, rIdx);
								octantsMatch = false;
								break;
							}
						}
					}

					if (octantsMatch)
						AM_TRACEF("Octants match");
					else
						AM_ERRF("Octants mismatch");

					//float m = customBound.m_Margin;
					AM_TRACEF("");
				}

#endif
				int d = 0;

#ifndef AM_STANDALONE
				phBoundGeometry* geometry = (phBoundGeometry*)(0x000000002241F820);
				/*	static gmAddress shrunkPolysAddr = gmAddress::Scan("E8 ?? ?? ?? ?? E9 DF 00 00 00 0F 2E 3D");
					static void (*shrunkPolysByMargin)(phBoundPolyhedron*, float, Vec3V*) =
						shrunkPolysAddr.GetCall().To<decltype(shrunkPolysByMargin)>();

					static gmAddress shrunkVertssAddr = gmAddress::Scan("E8 ?? ?? ?? ?? E9 C6 00 00 00 E8");
					static void (*shrunkVertsByMargin)(phBoundPolyhedron*, float, Vec3V*) =
						shrunkVertssAddr.GetCall().To<decltype(shrunkVertsByMargin)>();*/
#endif

						//geometry->m_Type = PH_BOUND_GEOMETRY; // Test...


						//geometry->ComputeNeighbors();


				{
					/*fiStreamPtr regular = fiStream::Create("C:/Users/falco/Desktop/f620_bound.obj");
					geometry->WriteObj(regular);

					fiStreamPtr shrunkVanila = fiStream::Create("C:/Users/falco/Desktop/f620_bound_shrunk_vanila.obj");
					geometry->WriteObj(shrunkVanila, true);

					geometry->SetMarginAndShrink();
					fiStreamPtr shrunkMy = fiStream::Create("C:/Users/falco/Desktop/f620_bound_shrunk_my.obj");
					geometry->WriteObj(shrunkMy, true);*/

#ifdef AM_STANDALONE
					//fiStreamPtr regular = fiStream::Create("C:/Users/falco/Desktop/rock_bound.obj");
					//geometry->WriteObj(regular);

					//fiStreamPtr shrunk = fiStream::Create("C:/Users/falco/Desktop/rock_bound_shrunk.obj");
					//geometry->WriteObj(shrunk, true);


					//Vec3V* oldShrunkVerts = new Vec3V[bg->m_NumVertices];
					//for (u32 i = 0; i < bg->m_NumVertices; i++)
					//{
					//	oldShrunkVerts[i] = bg->DecompressVertex(i, true);
					//}

					//Vec3V* newShrunkVerts = new Vec3V[geometry->m_NumVertices];
					{
						//// Shrunk polygons
						//geometry->ShrinkPolysByMargin(0.04f, newShrunkVerts);
						//for (u32 i = 0; i < geometry->m_NumVertices; i++)
						//{
						//	geometry->m_CompressedShrunkVertices.Get()[i] = geometry->CompressVertex(newShrunkVerts[i]);
						//}
						//geometry->SetMarginAndShrink(0.04);

						//	fiStreamPtr shrunkMine = fiStream::Create("C:/Users/falco/Desktop/rock_bound_shrunk_mine.obj");
						//	geometry->WriteObj(shrunkMine, true);
					}

					//// Compare
					//float maxDelta = 0.0f;
					//for (u32 i = 0; i < geometry->m_NumVertices; i++)
					//{
					//	Vec3V d = (/*newShrunkVerts[i]*/geometry->DecompressVertex(i, true) - oldShrunkVerts[i]);
					//	float delta = abs(MAX(d.X(), MAX(d.Y(), d.Z())));
					//	maxDelta = MAX(delta, maxDelta);
					//}
					//AM_TRACEF("MAX DELTA: %f", maxDelta);

					//delete newShrunkVerts;
					//delete oldShrunkVerts;
#endif

#ifndef AM_STANDALONE
					//{
					//	// Shrunk polygons
					//	Vec3V* shrunkVerts = new Vec3V[geometry->m_NumVertices];
					//	shrunkPolysByMargin(geometry, 0.04f, shrunkVerts);
					//	for (u32 i = 0; i < geometry->m_NumVertices; i++)
					//	{
					//		geometry->m_CompressedShrunkVertices.Get()[i] = geometry->CompressVertex(shrunkVerts[i]);
					//	}
					//	delete shrunkVerts;

					//	fiStreamPtr shrunk = fiStream::Create("C:/Users/falco/Desktop/shrunk_polys_game.obj");
					//	geometry->WriteObj(shrunk, true);
					//}

					//{
					//	// Shrunk polygons
					//	Vec3V* shrunkVerts = new Vec3V[geometry->m_NumVertices];
					//	shrunkVertsByMargin(geometry, 0.04f, shrunkVerts);
					//	for (u32 i = 0; i < geometry->m_NumVertices; i++)
					//	{
					//		geometry->m_CompressedShrunkVertices.Get()[i] = geometry->CompressVertex(shrunkVerts[i]);
					//	}
					//	delete shrunkVerts;

					//	fiStreamPtr shrunk = fiStream::Create("C:/Users/falco/Desktop/shrunk_verts.obj");
					//	geometry->WriteObj(shrunk, true);
					//}
#endif
				}

				//for (u32 i = 0; i < geometry->m_NumPolygons; i++)
				//{
				//	phPolygon& poly = geometry->GetPolygon(i);

				//	u16 v1 = poly.GetVertexIndex(0);
				//	u16 v2 = poly.GetVertexIndex(1);
				//	u16 v3 = poly.GetVertexIndex(2);

				//	u16 n1 = poly.GetNeighborIndex(0);
				//	u16 n2 = poly.GetNeighborIndex(1);
				//	u16 n3 = poly.GetNeighborIndex(2);

				//	AM_TRACEF("F VertexIndices %u %u %u NeighborFaceIndices %u %u %u", v1, v2, v3, n1, n2, n3);
				//}

				// Export OBJs
				AM_TRACE("");
				}

			/*	if (ImGui::Button("Drawable Tessellated"))
				{
					gtaDrawable* drawable;
					rage::pgRscBuilder::Load(&drawable, "C://Users//falco//Desktop//prop_palm_fan_03_c.ydr", 165);
				}*/

				//ImGui::End();
				//return;
	//#endif

			if (0) // Hint text box test
			{
				static const char* hints[] =
				{
					"Apple", "Orange", "Pigeon", "Cucumber", "Potato"
				};

				auto iteratorFn = [](int index, const char** hint, ImTextureID* icon, ImVec2* iconSize, float* iconScale)
					{
						*hint = hints[index];
					};

				ImGui::SetNextItemWidth(-1);

				//const char* selectedHint = hints[sleectedHint];
				//SlPickerState pickerState = SlGui::InputPicker("##Test1", selected, 5, iteratorFn);

				//if (pickerState. != -1)
				//{
				//	AM_TRACEF("%u", pickedHint);
				//	sleectedHint = pickedHint;
				//}
				ImGui::End();
				return;
			}

			//if (ImGui::Button("Load Effect"))
			//{
			//	rage::grcEffect effect;
			//	rage::grcEffectMgr::LoadFromFile(&effect, "C:/Users/falco/Desktop/normal_spec.fxc");
			//}

			bool needReload = false;

			static float R = 0;
			static float G = 0;
			static float B = 0;
			static float A = 1;

			if (ImGui::DragFloat("R", &R, 0.01f, 0, 1)) needReload = true;
			if (ImGui::DragFloat("G", &G, 0.01f, 0, 1)) needReload = true;
			if (ImGui::DragFloat("B", &B, 0.01f, 0, 1)) needReload = true;
			if (ImGui::DragFloat("A", &A, 0.01f, 0, 1)) needReload = true;

#ifdef dsdadaaadas
			if (ImGui::Button("Load GL") || needReload)
			{
#ifndef AM_STANDALONE
				grcVertexProgram* shader = nullptr;

				int shaderIndex = -1;
				if (shaderIndex == -1)
				{/*
					const char* SHADER_NAME = "normal";
					const char* PROGRAM_NAME = "normal:VS_Transform";*/
					const char* SHADER_NAME = "cable";
					const char* PROGRAM_NAME = "cable:VS_Cable";

					rage::grcEffect* shaderEffect = nullptr;
					for (int i = 0; i < 512; i++)
					{
						shaderEffect = effects[i];
						if (shaderEffect->m_NameHash == rage::joaat(SHADER_NAME))
						{
							for (rage::grcVertexProgram& program : shaderEffect->m_VertexPrograms)
							{
								if (String::Equals(program.m_Name, PROGRAM_NAME))
								{
									shader = &program;
									break;
								}
							}
							break;
						}
					}
				}
				else
				{
					/*auto vsIndex = effects[shaderIndex]->m_Techniques[shaderDrawIndex].m_Passes[0].VS;
					shader = &effects[shaderIndex]->m_VertexPrograms[vsIndex];*/
				}

				ID3D11ShaderReflection* reflection;
				HRESULT result =
					D3DReflect(shader->m_Bytecode, shader->m_BytecodeSize, IID_ID3D11ShaderReflection, (void**)&reflection);

				D3D11_SHADER_DESC desc;
				reflection->GetDesc(&desc);

				D3D11_SIGNATURE_PARAMETER_DESC signatureParameterDescs[16];
				for (int i = 0; i < desc.InputParameters; i++)
					reflection->GetInputParameterDesc(i, &signatureParameterDescs[i]);

				D3D11_INPUT_ELEMENT_DESC elementDescs[16];
				u16 elementCount = 0;
				for (int i = 0; i < desc.InputParameters; i++)
				{
					if (signatureParameterDescs[i].SystemValueType == D3D_NAME_INSTANCE_ID)
						continue;

					elementCount++;

					D3D11_SIGNATURE_PARAMETER_DESC& paramDesc = signatureParameterDescs[i];
					D3D11_INPUT_ELEMENT_DESC& elementDesc = elementDescs[i];

					// fill out input element desc
					elementDesc.SemanticName = paramDesc.SemanticName;
					elementDesc.SemanticIndex = paramDesc.SemanticIndex;
					elementDesc.InputSlot = 0;
					elementDesc.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;;
					elementDesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
					elementDesc.InstanceDataStepRate = 0;

					// determine DXGI format
					if (paramDesc.Mask == 1)
					{
						if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32_UINT;
						else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32_SINT;
						else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32_FLOAT;
					}
					else if (paramDesc.Mask <= 3)
					{
						if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32G32_UINT;
						else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32G32_SINT;
						else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
					}
					else if (paramDesc.Mask <= 7)
					{
						if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_UINT;
						else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_SINT;
						else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
					}
					else if (paramDesc.Mask <= 15)
					{
						if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
						else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
						else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
					}
				}

				rage::grcVertexDeclaration* decl =
					rage::grcDevice::CreateVertexDeclaration(elementDescs, elementCount);
				rage::grcFvf fvf;
				decl->EncodeFvf(fvf);

#else
				rage::grcFvf fvf{};
				fvf.Channels |= 1 << CHANNEL_POSITION;
				fvf.Channels |= 1 << CHANNEL_NORMAL;
				fvf.Channels |= 1 << CHANNEL_COLOR0;
				fvf.Channels |= 1 << CHANNEL_TEXCOORD0;
				fvf.Channels |= 1 << CHANNEL_TANGENT0;
				fvf.Stride = sizeof Vector3 + sizeof Vector3 + sizeof u32 + sizeof Vector2 + sizeof Vector4;
				fvf.ChannelCount = 5;
				fvf.SetFormat(CHANNEL_POSITION, GRC_FORMAT_R32G32B32_FLOAT);
				fvf.SetFormat(CHANNEL_NORMAL, GRC_FORMAT_R32G32B32_FLOAT);
				fvf.SetFormat(CHANNEL_COLOR0, GRC_FORMAT_R8G8B8A8_UNORM);
				fvf.SetFormat(CHANNEL_TEXCOORD0, GRC_FORMAT_R32G32_FLOAT);
				fvf.SetFormat(CHANNEL_TANGENT0, GRC_FORMAT_R32G32B32A32_FLOAT);

				auto decl = rage::grcVertexDeclaration::CreateFromFvf(fvf);
#endif

				model::ModelFileGl modelFile;
				modelFile.Load(L"C:/Users/falco/Desktop/cable.gltf");
				modelFile.SetVertexFormat(decl);

				model::MeshNodePtr& rootNode = modelFile.GetRootNode();

#ifndef AM_STANDALONE
				// Replace model
				rmcLodGroup& lods = drawable->GetLodGroup();
				rmcLod* lod = lods.GetLod(LOD_HIGH);

				static char oldMemory[sizeof atArray<grmModel>];

				pgUPtrArray<grmModel>& models = lod->GetModels();
				// Extreme hack!
				memset(&models, 0, sizeof atArray<grmModel>);

				static float emissiveValue = 65.0f;

				// SET NEW EFFECT
	/*			auto& mat = drawable->m_ShaderGroup->m_Materials[0];
				mat->m_Effect = shaderEffect;*/
				/*rage::grcInstanceVar* vars = mat->m_Variables.Get();
				grcInstanceVar& emissive = vars[4];
				emissive.dataCount = 1;
				emissive.Data = (char*)&emissiveValue;*/
#endif

				for (model::MeshNodePtr& child : rootNode->Children)
				{
					// For now only single depth... don't account sub-children
					model::ModelPtr modelPtr = modelFile.ReadModel(child);

					rage::grmModel* model = new rage::grmModel();
					for (model::ModelPrimitivePtr& primitive : modelPtr->Primitives)
					{
						rage::pgUPtr geometry = new rage::grmGeometryQB();

						auto vertices = primitive->Vertices.get();
						auto indices = primitive->Indices.get();
						u32 vertexCount = primitive->VertexCount;
						u32 indexCount = primitive->IndexCount;

						// TODO: Cable Test
						for (u32 i = 0; i < vertexCount - 3; i += 3)
						{
							struct Vertex
							{
								Vector3 Position;
								Vector3 Normal;
								Vector4	Color;
								Vector2	TexCoord;
							};
							static_assert(sizeof Vertex == 48);

							Vertex* vtxArray = (Vertex*)vertices;
							Vertex& vtx = vtxArray[i];

							vtx.Color = Vector4(R, G, B, A);

							//Vec3V from = vtxArray[i].Position;
							//Vec3V to = vtxArray[i + 2].Position;
							//Vec3V norm = from - to;
							//norm.Normalize();

							//vtx.Normal = Vector3(norm.X, norm.Y, norm.Z);
						}

						geometry->SetVertexData(
							vertices, vertexCount, indices, indexCount, decl, &fvf);

						model->AddGeometry(geometry, primitive->AABB);
					}
					//model->TestTessellation();

					model->ComputeAABB();
					// grmModel->SortForTesselation()
#ifndef AM_STANDALONE
					models.Emplace(std::move(model));
			}
}
#endif
#endif
			if (ImGui::Button("YTD"))
			{
				rage::grcTextureDictionary* td1;
				//rage::pgRscBuilder::Load(&td1, "C:\Users\falco\Desktop\big.itd", 13);

				//rage::grcTextureDictionary* td1;
				//rage::pgRscBuilder::Load(&td1, "C:/Users/falco/Desktop/Assets/adder.ytd", 13);

				//rage::grcTextureDictionary* td2;
				//rage::pgRscBuilder::Load(&td2, "C:/Users/falco/Desktop/adder.ytd", 13);


				//for (int i = 0; i < td1->GetSize(); i++)
				//	AM_TRACEF(td1->GetAt(i).Value->GetName());

				//AM_TRACEF("");

				//for (int i = 0; i < td2->GetSize(); i++)
				//	AM_TRACEF(td2->GetAt(i).Value->GetName());

				AM_TRACEF("");
			}

			//if (ImGui::Button("Model File"))
			//{
			//	model::ModelFileGl modelFile;
			//	modelFile.Load(L"C:/Users/falco/Desktop/demo.gltf");

			//	// TODO: Format builder
			//	rage::grcFvf format{};
			//	format.Channels |= 1 << 0; // POSITION
			//	format.Channels |= 1 << 3; // NORMAL
			//	format.Channels |= 1 << 4; // COLOR0
			//	format.Channels |= 1 << 6; // TEXCOORD0
			//	format.Stride = sizeof rage::Vector3 + sizeof rage::Vector3 + sizeof u32 + sizeof rage::Vector2;
			//	format.ChannelCount = 4;
			//	format.ChannelFormats |= rage::GRC_FORMAT_R32G32B32_FLOAT;
			//	format.ChannelFormats |= rage::GRC_FORMAT_R32G32B32_FLOAT << 12;
			//	format.ChannelFormats |= rage::GRC_FORMAT_R8G8B8A8_UNORM << 16;
			//	format.ChannelFormats |= rage::GRC_FORMAT_R32G32_FLOAT << 24;

			//	//modelFile.SetVertexFormat(&format);

			//	// Print hierarchy
			//	std::function<void(const model::MeshNodePtr&)> printNode;
			//	printNode = [&](const model::MeshNodePtr& node)
			//		{
			//			AM_TRACEF("%s", node->Name);
			//			for (auto& child : node->Children)
			//				printNode(child);
			//		};
			//	printNode(modelFile.GetRootNode());

			//	//auto node = model.GetRootNode()->Children[0];
			//	//char* vertices = new char[format.Stride * model.GetVertexCount(node)];

			//	//model.ReadVertices(node, vertices);

			//	//delete[] vertices;

			//	auto model = modelFile.ReadModel(modelFile.GetRootNode()->Children[0]);

			//	AM_TRACEF("");
			//}

			static float margin = 0.04f;
			static float interpolation = 0.0f;
			ImGui::InputFloat("Margin", &margin);
			ImGui::DragFloat("Interpolation", &interpolation, 0.1f, 0.0f, 1.0f);
			if (ImGui::Button("Set Margin"))
			{
				u64 bound = 0x00000001244094E0;

				static gmAddress setMarginAndShrinkAddr = gmAddress::Scan(
					"48 8B C4 48 89 58 18 55 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 B0 48 81 EC 50 01 00 00 0F 28 41 20");
				static auto setMarginAndShrinkFn = setMarginAndShrinkAddr.To<void(*)(u64, float, float)>();

				setMarginAndShrinkFn(bound, margin, interpolation);
			}

			//if (ImGui::Button("Load Custom Mesh"))
			//{
			//	struct Vertex
			//	{
			//		rage::Vector3 Pos;
			//		rage::Vector3 Normal;
			//		u8 A, R, G, B;
			//		rage::Vector2 TexCoors;
			//		rage::Vector4 Tangent;
			//	};

			//	rage::atArray<Vertex, u32> vertices;
			//	rage::atArray<u16, u32> indices;

			//	cgltf_options options{};
			//	cgltf_data* data = NULL;
			//	cgltf_result result = cgltf_parse_file(&options, "C:/Users/falco/Desktop/monkey.glb", &data);
			//	if (result == cgltf_result_success)
			//	{
			//		/* TODO make awesome stuff */
			//		cgltf_load_buffers(&options, data, NULL);


			//		cgltf_mesh& mesh = data->meshes[0];
			//		cgltf_primitive& primitive = mesh.primitives[0];
			//		vertices.Resize(primitive.indices->count);
			//		for (auto& vtx : vertices)
			//		{
			//			vtx.Tangent = { 1.0f, 0.0f, 0.0f, 1.0f }; // TODO: Compute if missing
			//			vtx.A = 125;
			//			vtx.R = 125;
			//			vtx.G = 125;
			//			vtx.B = 125;
			//		}

			//		for (cgltf_size i = 0; i < primitive.attributes_count; i++)
			//		{
			//			cgltf_attribute& attribute = primitive.attributes[i];
			//			if (attribute.type == cgltf_attribute_type_position)
			//			{
			//				auto& view = attribute.data->buffer_view;
			//				auto values = (rage::Vector3*)((char*)view->buffer->data + view->offset);
			//				auto posCount = view->size / sizeof rage::Vector3;
			//				for (cgltf_size k = 0; k < posCount; k++)
			//					vertices[k].Pos = values[k];
			//			}
			//			if (attribute.type == cgltf_attribute_type_normal)
			//			{
			//				auto& view = attribute.data->buffer_view;
			//				auto normals = (rage::Vector3*)((char*)view->buffer->data + view->offset);
			//				auto cunt = view->size / sizeof rage::Vector3;
			//				for (cgltf_size k = 0; k < cunt; k++)
			//					vertices[k].Normal = normals[k];
			//			}
			//			if (attribute.type == cgltf_attribute_type_tangent)
			//			{
			//				auto& view = attribute.data->buffer_view;
			//				auto normals = (rage::Vector4*)((char*)view->buffer->data + view->offset);
			//				auto cunt = view->size / sizeof rage::Vector4;
			//				for (cgltf_size k = 0; k < cunt; k++)
			//					vertices[k].Tangent = normals[k];
			//			}
			//			if (attribute.type == cgltf_attribute_type_texcoord)
			//			{
			//				auto& view = attribute.data->buffer_view;
			//				auto normals = (rage::Vector2*)((char*)view->buffer->data + view->offset);
			//				auto cunt = view->size / sizeof rage::Vector2;
			//				for (cgltf_size k = 0; k < cunt; k++)
			//					vertices[k].TexCoors = normals[k];
			//			}
			//		}
			//		// TODO: Indices type can be different
			//		auto& indView = primitive.indices->buffer_view;
			//		indices.Reserve(indView->size / sizeof u16);
			//		auto inx = (u16*)((char*)indView->buffer->data + indView->offset);
			//		for (auto i = 0; i < indView->size / sizeof u16; i++)
			//		{
			//			indices.Add(inx[i]);
			//		}/*
			//		for (auto i = 0; i < vertices.GetSize(); i++)
			//			indices.Add(i);*/

			//		auto device = rageam::render::Engine::GetInstance()->GetFactory();
			//		ID3D11Buffer* vtxBuffer;
			//		D3D11_BUFFER_DESC vtxDesc{};
			//		vtxDesc.Usage = D3D11_USAGE_DEFAULT;
			//		vtxDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			//		vtxDesc.StructureByteStride = sizeof Vertex;//68;
			//		vtxDesc.ByteWidth = sizeof Vertex * vertices.GetSize();
			//		D3D11_SUBRESOURCE_DATA vtxData{};
			//		vtxData.pSysMem = vertices.GetItems();
			//		device->CreateBuffer(&vtxDesc, &vtxData, &vtxBuffer);

			//		ID3D11Buffer* idxBuffer;
			//		D3D11_BUFFER_DESC idxDesc{};
			//		idxDesc.Usage = D3D11_USAGE_DEFAULT;
			//		idxDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
			//		idxDesc.StructureByteStride = sizeof u16;//68;
			//		idxDesc.ByteWidth = sizeof u16 * indices.GetSize();
			//		D3D11_SUBRESOURCE_DATA idxData{};
			//		idxData.pSysMem = indices.GetItems();
			//		device->CreateBuffer(&idxDesc, &idxData, &idxBuffer);

			//		cgltf_free(data);

			//		////static char drawableName[64] = "prop_palm_fan_03_c";
			//		//static char drawableName[64] = "prop_metal_plates02";
			//		//rage::strLocalIndex slot;
			//		//rage::strStreamingModule* module = hooks::Streaming::GetModule("ydr");
			//		//module->GetSlotIndexByName(slot, drawableName);
			//		//auto dr = (gtaDrawable*)(module->GetPtr(slot));


			//		/*for (auto& lod : dr->m_LodGroup.m_LodModels)
			//		{
			//			if (lod == nullptr)
			//				continue;

			//			u64 vtxDecl = 0;
			//			for (auto& model : lod->GetModels())
			//			{
			//				for (auto& geometry : model->GetGeometries())
			//				{
			//					if (vtxDecl == 0)
			//						vtxDecl = geometry->m_VertexDeclaration;

			//					geometry->m_QuadTreeData = 0;
			//					geometry->m_IndexCount = indices.GetSize();
			//					geometry->m_PrimitiveCount = vertices.GetSize() / 3;
			//					geometry->m_VertexStride = sizeof Vertex;
			//					geometry->m_VertexDeclaration = vtxDecl;
			//					geometry->m_VertexBuffers[0]->m_Buffer.Buffer = vtxBuffer;
			//					geometry->m_IndexBuffers[0]->m_Buffer.Buffer = idxBuffer;
			//					geometry->m_VertexBuffers[0]->m_VertexStride = sizeof Vertex;
			//				}
			//			}
			//		}*/
			//	}
			//}


			//// Streaming
			//{
			//	rage::strStreamingInfo* info = hooks::Streaming::GetInfo(module->GetGlobalIndex(slot));
			//	if (module && info)
			//	{

			//		ImGui::Text("adder.ytd");
			//		ImGui::Text("Slot: %i, Streamed: %s, Data: %x",
			//			slot,
			//			module->GetResource(slot) ? "True" : "False",
			//			info->Data);
			//	}
			//}

			//if (SlGui::Button("Stream"))
			//{
			//	auto txd = asset::AssetFactory::LoadFromPath<asset::TxdAsset>(
			//		"C:/Users/falco/Desktop/Assets/adder.itd");

			//	if (txd && txd->CompileToGame(&s_Txd))
			//	{
			//		module->Set(slot, &s_Txd);
			//		rage::strStreamingInfo* info = hooks::Streaming::GetInfo(module->GetGlobalIndex(slot));

			//		info->Data = 0x8000009; // Mark as loaded
			//	}
			//}
			//ImGui::SameLine();

			//if (SlGui::Button("Tests"))
			//{
			//	rage::atMap<ConstString, ConstString> test;
			//	test.InitAndAllocate(10000);
			//	test.Insert("Hello", "Hello");
			//}
			//ImGui::SameLine();
			//if (SlGui::Button("Compile Asset"))
			//{
			//	amPtr<asset::TxdAsset> txd = asset::AssetFactory::LoadFromPath<asset::TxdAsset>(
			//		"C:/Users/falco/Source/Repos/rageAm/rageAm/Resources/ranstar.pack/ranstar.itd");

			//	if (txd)
			//		txd->CompileToFile("C:/Users/falco/Source/Repos/rageAm/rageAm/Resources/ranstar.pack/ranstar.ytd");
			//}
			//ImGui::SameLine();
			//if (SlGui::Button("Create Asset"))
			//{
			//	asset::TxdAsset txd("C:/Users/falco/Source/Repos/rageAm/rageAm/Resources/ranstar.pack/ranstar.itd");
			//	txd.Refresh();
			//	txd.SaveConfig();
			//}

			ImGui::End();
		}
	};
}
