#include "modelinspector.h"

#ifdef AM_INTEGRATED

#include "widgets.h"
#include "am/file/fileutils.h"
#include "am/graphics/color.h"
#include "am/ui/imglue.h"
#include "am/ui/font_icons/icons_am.h"
#include "helpers/format.h"
#include "rage/paging/builder/builder.h"

ConstString rageam::integration::ModelInspector::FormatVec(const Vec3V& vec) const
{
	return ImGui::FormatTemp("<%.02f  %.02f  %.02f>", vec.X(), vec.Y(), vec.Z());
}

bool rageam::integration::ModelInspector::IconButton(ConstString name) const
{
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
	bool v = ImGui::Button(name);
	ImGui::PopStyleVar(2);
	return v;
}

void rageam::integration::ModelInspector::DisabledCheckBox(ConstString name, bool v) const
{
	ImGui::TableNextColumn();
	ImGui::BeginDisabled();
	ImGui::TablePushBackgroundChannel(); // Span across all columns
	ImGui::Checkbox(name, &v);
	ImGui::TablePopBackgroundChannel();
	ImGui::EndDisabled();

	ImGui::TableNextColumn();
	ImGui::TableNextColumn();
}

void rageam::integration::ModelInspector::PropertyValue(ConstString propertyName, ConstString valueFormat, ...)
{
	ImGuiID id = ImGui::GetID(propertyName);

	ImGui::TableNextColumn();
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.85f); // Grayed-out text helps to see categories and values better
	ImGui::SetNextItemAllowOverlap(); // To allow buttons overlap it
	if (ImGui::Selectable(propertyName, m_SelectedRowID == id, ImGuiSelectableFlags_SpanAllColumns))
		m_SelectedRowID = id;
	ImGui::PopStyleVar();
	ImGui::TableNextColumn();
	va_list args;
	va_start(args, valueFormat);
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.85f); // Bold looks way too bright
	ImGui::PushFont(ImFont_Medium);
	ImGui::TextV(valueFormat, args);
	ImGui::PopFont();
	ImGui::PopStyleVar();
	va_end(args);
	ImGui::TableNextColumn();
	// User can pass custom widgets after...
}

bool rageam::integration::ModelInspector::BeginCategory(ConstString name, bool defaultOpen)
{
	ImU32 colors[] =
	{
		graphics::ColorU32(240, 185, 60),  // Yellow
		graphics::ColorU32(100, 175, 230), // Blue
		graphics::ColorU32(235, 100, 185), // Pink
	};

	ImGuiID id = ImGui::GetID(name);
	ImGuiTreeNodeFlags flags = 
		ImGuiTreeNodeFlags_SpanAllColumns | 
		ImGuiTreeNodeFlags_FramePadding;
	if (defaultOpen)
		flags |= ImGuiTreeNodeFlags_DefaultOpen;
	if (m_SelectedRowID == id)
		flags |= ImGuiTreeNodeFlags_Selected;
	
	ImGui::TableNextColumn();
	bool opened;
	{
		// int colorIndex = ImGui::GetCurrentWindow()->DC.TreeDepth % std::size(colors);

		ImGui::PushFont(ImFont_Medium);
		// ImGui::PushStyleColor(ImGuiCol_Text, colors[colorIndex]);
		// ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		opened = ImGui::TreeNodeEx(name, flags);
		// ImGui::PopStyleVar();
		// ImGui::PopStyleColor();
		ImGui::PopFont();

		if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
			m_SelectedRowID = id;
	}
	ImGui::TableNextColumn();
	ImGui::TableNextColumn();
	return opened;
}

void rageam::integration::ModelInspector::EndCategory() const
{
	ImGui::TreePop();
}

void rageam::integration::ModelInspector::DrawGeometry(rage::grmModel* model, u16 index)
{
	auto& geometry = model->GetGeometries().Get(index);

	if (!BeginCategory(ImGui::FormatTemp("Geometry #%i", index)))
		return;

	PropertyValue("Material Index", "%u", model->GetMaterialIndex(index));
	if (IconNav())
		m_MaterialIndexToSelect = model->GetMaterialIndex(index);
	
	PropertyValue("Vertex Count", "%u", geometry->GetVertexCount());
	PropertyValue("Index Count", "%u", geometry->GetIndexCount());

	if (BeginCategory("Vertex Format"))
	{
		rage::grcVertexElement vertexElements[rage::grcFvf::MAX_CHANNELS];
		rage::grcFvf fvf = *geometry->GetFvf();
		u32 elementCount = fvf.Unpack(vertexElements);

		for (u32 i = 0; i < elementCount; i++)
		{
			rage::grcVertexElement& element = vertexElements[i];

			ConstString name = rage::VertexSemanticName[element.Semantic];
			ConstString formatName = Enum::GetName(element.Format) + 11; // Skip 'GRC_FORMAT_'
			PropertyValue(name, formatName);
		}

		EndCategory();
	}

	EndCategory();
}

void rageam::integration::ModelInspector::DrawLodGroup()
{
	rage::rmcLodGroup& lodGroup = m_Drawable->GetLodGroup();
	for (int i = 0; i < lodGroup.GetLodCount(); i++)
	{
		rage::rmcLod*    lod = lodGroup.GetLod(i);
		rage::grmModels& lodModels = lod->GetModels();

		// No models, skip this empty lod...
		if (!lodModels.Any())
			continue;

		ConstString categoryName = ImGui::FormatTemp("LOD #%i, Models: %u", i, lodModels.GetSize());
		if (!BeginCategory(categoryName))
			continue;

		PropertyValue("Threshold", "%.02f", lodGroup.GetLodThreshold(i));
		ImGui::HelpMarker("Maximum render distance in meters.");

		for (u16 k = 0; k < lodModels.GetSize(); k++)
		{
			if (!BeginCategory(ImGui::FormatTemp("Model #%u", k)))
				continue;

			auto& model = lodModels[k];
			// Draw Mask
			//if (BeginCategory("Sub Draw Buckets", true))
			//{
			//	u32 subDrawBucketMask = model->GetSubDrawBucketMask();
			///*	if (subDrawBucketMask & 1 << rage::RB_MODEL_DEFAULT)			   PropertyValue("Is Visible", "");
			//	if (subDrawBucketMask & 1 << rage::RB_MODEL_SHADOW)				   PropertyValue("Has Shadows", "");
			//	if (subDrawBucketMask & 1 << rage::RB_MODEL_TESSELLATION)		   PropertyValue("Tessellated", "");
			//	if (subDrawBucketMask & 1 << rage::RB_MODEL_REFLECTION_PARABOLOID) PropertyValue("Paraboloid Reflections", "");
			//	if (subDrawBucketMask & 1 << rage::RB_MODEL_REFLECTION_MIRROR)	   PropertyValue("Mirror Reflections", "");
			//	if (subDrawBucketMask & 1 << rage::RB_MODEL_REFLECTION_WATER)	   PropertyValue("Water Reflections", "");*/

			//	DisabledCheckBox("RB_MODEL_DEFAULT", subDrawBucketMask & 1 << rage::RB_MODEL_DEFAULT);
			//	DisabledCheckBox("RB_MODEL_SHADOW", subDrawBucketMask & 1 << rage::RB_MODEL_SHADOW);
			//	DisabledCheckBox("RB_MODEL_TESSELLATION", subDrawBucketMask & 1 << rage::RB_MODEL_TESSELLATION);
			//	DisabledCheckBox("RB_MODEL_REFLECTION_PARABOLOID", subDrawBucketMask & 1 << rage::RB_MODEL_REFLECTION_PARABOLOID);
			//	DisabledCheckBox("RB_MODEL_REFLECTION_MIRROR", subDrawBucketMask & 1 << rage::RB_MODEL_REFLECTION_MIRROR);
			//	DisabledCheckBox("RB_MODEL_REFLECTION_WATER", subDrawBucketMask & 1 << rage::RB_MODEL_REFLECTION_WATER);

			//	EndCategory();
			//}

			PropertyValue("Sub Draw Mask", "%x", model->GetSubDrawBucketMask());
			// SetNextPropertyToolTip("Uses mesh deformation technology to blend between 4 bones.");
			PropertyValue("Is Skinned", "%s", model->IsSkinned() ? "True" : "False");
			// SetNextPropertyToolTip("Bone this model is attached to, only for non-skinned (with deformation) models.");
			PropertyValue("Bone Index", "%u", model->GetBoneIndex());
			PropertyValue("Geometry Count", "");
			ImGui::Indent();
			PropertyValue("Total", "%u", model->GetGeometryCount());
			PropertyValue("Tessellated", "%u", model->GetTesselatedGeometryCount());
			ImGui::Unindent();

			auto& geometries = model->GetGeometries();
			for (u16 j = 0; j < geometries.GetSize(); j++)
			{
				DrawGeometry(model.Get(), j);
			}

			EndCategory(); // Lod Model
		}
		EndCategory(); // Lod
	}
}

void rageam::integration::ModelInspector::DrawShaderParams(const rage::grmShader* shader, rage::grcEffect* effect)
{
	if (!BeginCategory("Params"))
		return;

	for (u16 j = 0; j < shader->GetVarCount(); j++)
	{
		rage::grcEffectVar* varInfo = effect->GetVar(j);
		rage::grcInstanceVar* var = shader->GetVar(j);

		int*   iVec = var->GetValuePtr<int>();
		float* fVec = var->GetValuePtr<float>();

		ConstString varValue = "-";
		switch (varInfo->GetType())
		{
			case rage::VT_INT:
			case rage::VT_INT1:		varValue = ImGui::FormatTemp("%i", var->GetValue<int>());											break;
			case rage::VT_INT2:		varValue = ImGui::FormatTemp("<%i  %i>", iVec[0], iVec[1]);											break;
			case rage::VT_INT3:		varValue = ImGui::FormatTemp("<%i  %i  %i>", iVec[0], iVec[1], iVec[2]);							break;
			case rage::VT_FLOAT:	varValue = ImGui::FormatTemp("%.02f", var->GetValue<float>());										break;
			case rage::VT_VECTOR2:	varValue = ImGui::FormatTemp("<%.02f  %.02f>", fVec[0], fVec[1]);									break;
			case rage::VT_VECTOR3:	varValue = ImGui::FormatTemp("<%.02f  %.02f  %.02f>", fVec[0], fVec[1], fVec[2]);					break;
			case rage::VT_VECTOR4:	varValue = ImGui::FormatTemp("<%.02f  %.02f  %.02f  %.02f>", fVec[0], fVec[1], fVec[2], fVec[3]);	break;
			case rage::VT_BOOL:		varValue = var->GetValue<bool>() ? "True" : "False";												break;
			case rage::VT_TEXTURE:  varValue = var->GetTexture() ? var->GetTexture()->GetName() : "Null";								break;
			default: break;
		}

		PropertyValue(varInfo->GetName(), varValue);
	}

	EndCategory();
}

void rageam::integration::ModelInspector::DrawMaterialGroup()
{
	rage::grmShaderGroup* shaderGroup = m_Drawable->GetShaderGroup();
	for (u16 i = 0; i < shaderGroup->GetShaderCount(); i++)
	{
		rage::grmShader* shader = shaderGroup->GetShader(i);
		rage::grcEffect* effect = shader->GetEffect();
		ConstString shaderName = ImGui::FormatTemp("Material #%i", i);

		bool beganShader = BeginCategory(shaderName);

		// Handle this after drawing so we can use ScrollToItem
		if (m_MaterialIndexToSelect == i)
		{
			m_SelectedRowID = GImGui->LastItemData.ID;
			m_MaterialIndexToSelect = -1;
			ImGui::ScrollToItem();
		}

		if (beganShader)
		{
			PropertyValue("Effect Name", "%s", effect->GetName());
			ImGui::HelpMarker("Name of the .fxc file used by the material.");
			PropertyValue("Draw Bucket", "%u", shader->GetDrawBucket());
			PropertyValue("Draw Bucket Mask", "%x", shader->GetDrawBucketMask());

			DrawShaderParams(shader, effect);

			EndCategory();
		}
	}
}

void rageam::integration::ModelInspector::OnRender()
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(1, 1));
	bool drawInspector = ImGui::Begin("Model Inspector") && m_Drawable;
	ImGui::PopStyleVar();
	if (!drawInspector)
	{
		// Early exit, window is not visible or drawable is not loaded
		ImGui::End();
		return;
	}

	ImGuiTableFlags tableFlags =
		ImGuiTableFlags_Borders |
		ImGuiTableFlags_Resizable;
	if (ImGui::BeginTable("ModelInspectorTable", 3, tableFlags))
	{
		ImGui::TableSetupColumn("Name");
		ImGui::TableSetupColumn("Value");
		ImGui::TableSetupColumn("Extra", 
			ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed, 
			GImGui->FontSize);

		PropertyValue("Name", m_Drawable->GetName());

		if (BeginCategory("Resource"))
		{
			PropertyValue("File Size", FormatSize(m_FileSize));
			ImGui::HelpMarker("Size of the resource file on disk.");

			PropertyValue("Resource Size", FormatSize(m_ResourceSize));
			ImGui::HelpMarker("Size taken by streamed resource in-game");

			ImGui::Indent();
			PropertyValue("Virtual", FormatSize(m_VirtualSize));
			PropertyValue("Physical", FormatSize(m_PhysicalSize));
			ImGui::Unindent();
			EndCategory();
		}

		if (BeginCategory("Cull Bounds", true))
		{
			const rage::spdAABB&   bb = m_Drawable->GetBoundingBox();
			const rage::spdSphere& bs = m_Drawable->GetBoundingSphere();

			PropertyValue("Box Min", FormatVec(bb.Min));
			PropertyValue("Box Max", FormatVec(bb.Max));
			PropertyValue("Sphere Center", FormatVec(bs.GetCenter()));
			PropertyValue("Sphere Radius", "%.02f", bs.GetRadius().Get());

			EndCategory();
		}

		DrawLodGroup();
		DrawMaterialGroup();

		ImGui::EndTable();
	}
	ImGui::End();
}

rageam::integration::ModelInspector::ModelInspector()
{

}

void rageam::integration::ModelInspector::LoadFromPath(ConstWString path)
{
	static constexpr u32 ASSET_NAME_HASH = rage::atStringHash("amInspectorArchetype");

	if (!file::IsFileExists(path))
	{
		AM_ERRF(L"ModelInspector::LoadFromPath() -> File doesn't exists! '%ls'", path);
		return;
	}

	file::Path pathA = file::PathConverter::WideToUtf8(path);

	// We're using game place function, we must also use game allocator to not blow up things
	rage::sysMemUseGameAllocators(true);

	rage::datResourceMap map;
	rage::datResourceInfo info;
	rage::pgBase* root = rage::pgRscBuilder::LoadBuild(pathA, 165, map, info);
	
	m_Drawable = gtaDrawablePtr((gtaDrawable*) root);

	// Place drawable using native game function
	static auto placeFn = gmAddress::Scan(
#if APP_BUILD_2699_16_RELEASE_NO_OPT
		"48 8B 44 24 40 48 05 B0 00 00 00 41 B0 01", "gtaDrawable::gtaDrawable(const datResource& rsc)+0x2C").GetAt(-0x2C)
#else
		"66 3B BB B8 00 00 00", "gtaDrawable::gtaDrawable(const datResource& rsc)+0x47").GetAt(-0x47)
#endif
		.ToFunc<void(gtaDrawable*, rage::datResource*)>();

	// Minimize resource building scope, we use game loader functions for now,
	// although we may want to add switch to verify that our loading code is working properly
	{
		rage::datResource rsc(map, pathA);

		static auto datResourceCtor = gmAddress::Scan(
#if APP_BUILD_2699_16_RELEASE_NO_OPT
			"49 8B 44 00 18", "rage::datResource::datResource+0x196")
			.GetAt(-0x196)
#else
			"48 89 5C 24 08 57 48 83 EC 20 65 48 8B 04", "rage::datResource::datResource")
#endif
			.ToFunc<void(rage::datResource*, rage::datResourceMap*, const char*, bool)>();

		static auto datResourceDctor = gmAddress::Scan(
#if APP_BUILD_2699_16_RELEASE_NO_OPT
			"48 8B 52 08 48 89 14 08", "rage::datResource::~datResource+0x24")
			.GetAt(-0x24)
#else
			"42 FF 0C 00 C3", "rage::datResource::~datResource+0x25")
			.GetAt(-0x25)
#endif
			.ToFunc<void(rage::datResource*)>();

		datResourceCtor(&rsc, &map, pathA, false);
		placeFn(m_Drawable.Get(), &rsc);
		datResourceDctor(&rsc);
	}

	// End of the scope...
	rage::sysMemUseGameAllocators(false);

	m_FileSize = file::GetFileSize(path);
	m_VirtualSize = info.ComputeVirtualSize();
	m_PhysicalSize = info.ComputePhysicalSize();
	m_ResourceSize = m_VirtualSize + m_PhysicalSize;

	m_ArchetypeDef = std::make_shared<CBaseArchetypeDef>();
	m_ArchetypeDef->Name = ASSET_NAME_HASH;
	m_ArchetypeDef->AssetName = ASSET_NAME_HASH;
	m_ArchetypeDef->PhysicsDictionary = ASSET_NAME_HASH;
	m_ArchetypeDef->LodDist = 100.0f;
	m_ArchetypeDef->Flags = FLAG_IS_TYPE_OBJECT | FLAG_IS_FIXED | FLAG_HAS_ANIM;

	m_GameEntity.Create(m_Drawable, m_ArchetypeDef, Vec3V(-4, 75, 9));
	m_GameEntity->SetEntityWasAllocatedByGame(true); // Ensure that drawable will be destroyed with game allocator in TLS
}

#endif
