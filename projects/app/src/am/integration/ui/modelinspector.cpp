#include "modelinspector.h"

#ifdef AM_INTEGRATED

#include "widgets.h"
#include "am/file/fileutils.h"
#include "am/graphics/color.h"
#include "am/ui/imglue.h"
#include "am/ui/font_icons/icons_am.h"
#include "helpers/format.h"
#include "rage/paging/builder/builder.h"
#include "am/integration/gamedrawlists.h"
#include "am/ui/slwidgets.h"
#include "rage/physics/bounds/bvh.h"
#include "rage/physics/bounds/composite.h"

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

	char valueBuffer[256];
	va_list args;
	va_start(args, valueFormat);
	vsprintf_s(valueBuffer, sizeof valueBuffer, valueFormat, args);
	va_end(args);

	ImGui::TableNextColumn();
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.85f); // Grayed-out text helps to see categories and values better
	ImGui::SetNextItemAllowOverlap(); // To allow buttons overlap it
	if (ImGui::Selectable(propertyName, m_SelectedRowID == id, ImGuiSelectableFlags_SpanAllColumns))
		m_SelectedRowID = id;
	ImGui::ToolTip(valueBuffer);
	ImGui::PopStyleVar();
	ImGui::TableNextColumn();
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.85f); // Bold looks way too bright
	ImGui::PushFont(ImFont_Medium);
	ImGui::ScrollingLabel(GImGui->CurrentWindow->DC.CursorPos, GImGui->CurrentWindow->WorkRect, valueBuffer);
	ImGui::PopFont();
	ImGui::PopStyleVar();
	ImGui::TableNextColumn();
	// User can pass custom widgets after...
}

bool rageam::integration::ModelInspector::BeginCategory(ConstString name, bool defaultOpen, bool spanAllColumns)
{
	ImU32 colors[] =
	{
		graphics::ColorU32(240, 185, 60),  // Yellow
		graphics::ColorU32(100, 175, 230), // Blue
		graphics::ColorU32(235, 100, 185), // Pink
	};
	ImGuiID id = ImGui::GetID(name);

	bool selected = m_SelectedRowID == id;

	ImGuiTreeNodeFlags flags = 
		ImGuiTreeNodeFlags_SpanAllColumns | 
		ImGuiTreeNodeFlags_FramePadding | 
		ImGuiTreeNodeFlags_OpenOnDoubleClick;
	if (defaultOpen)
		flags |= ImGuiTreeNodeFlags_DefaultOpen;
	if (selected)
		flags |= ImGuiTreeNodeFlags_Selected;
	
	ImGui::TableNextColumn();
	bool opened;
	{
		// int colorIndex = ImGui::GetCurrentWindow()->DC.TreeDepth % std::size(colors);

		ImGui::PushFont(ImFont_Medium);
		// ImGui::PushStyleColor(ImGuiCol_Text, colors[colorIndex]);
		// ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		// opened = ImGui::TreeNodeEx(name, flags);
		bool toggled;
		if (GImGui->CurrentTable) ImGui::TablePushBackgroundChannel();
		ImGui::PushStyleColor(ImGuiCol_FrameBg, 0);
		// GImGui->CurrentTable->Flags &= ~ImGuiTableFlags_BordersInnerV; // TODO:
		opened = SlGui::TreeNode(name, selected, toggled, nullptr, flags);
		ImGui::PopStyleColor();
		if (GImGui->CurrentTable) ImGui::TablePopBackgroundChannel();
		if (selected)
			m_SelectedRowID = id;
		// opened = ImGui::TreeNodeBehavior(id, flags, "", NULL); // Empty label so we can use scrolling label
		ImGui::ToolTip(name);
		// ImRect textRect = GImGui->CurrentWindow->WorkRect;
		// textRect.Min = GImGui->CurrentWindow->DC.CursorPos;
		// ImGui::ScrollingLabel(textRect.Min + GImGui->Style.FramePadding, textRect, name);
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

bool rageam::integration::ModelInspector::BeginProperties() const
{
	ImGuiTableFlags tableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable;
	if (!ImGui::BeginTable("ModelInspectorTable", 3, tableFlags))
		return false;

	ImGui::TableSetupColumn("Name");
	ImGui::TableSetupColumn("Value");
	ImGui::TableSetupColumn("Extra", ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed, GImGui->FontSize);

	return true;
}

void rageam::integration::ModelInspector::EndProperties() const
{
	ImGui::EndTable();
}

ConstString rageam::integration::ModelInspector::FormatMaterialName(int index) const
{
	rage::grmShader* shader = m_Drawable->GetShaderGroup()->GetShader(index);
	rage::grcEffect* effect = shader->GetEffect();

	// Use the first texture name (diffuse) as material name
	// Textures are always placed first
	AM_DEBUG_ASSERTS(shader->GetVarCount() > 0);
	AM_DEBUG_ASSERTS(shader->GetVar(0)->IsTexture());
	const char* firstTextureName = shader->GetVar(0)->GetTexture()->GetName(); // TODO: Texture is NULL on some props, WHY?

	return ImGui::FormatTemp("%s/%s###Shader%i", effect->GetName(), firstTextureName, index);
}

void rageam::integration::ModelInspector::DrawBoundRecurse(rage::phBound* bound, Mat44V transform, rage::phBoundComposite* compositeParent, int compositeIndex)
{
	if (!bound)
		return;

	rage::phBoundComposite* composite = nullptr;
	if (bound->GetShapeType() == rage::PH_BOUND_COMPOSITE)
		composite = reinterpret_cast<rage::phBoundComposite*>(bound);

	ConstString boundTypeName = rage::phBoundTypeName[bound->GetShapeType()];
	ConstString treeNodeName = ImGui::FormatTemp("%s###BOUND_%p", boundTypeName, bound);
	SlGuiTreeNodeFlags treeNodeFlags = SlGuiTreeNodeFlags_None;

	if (!composite || composite->GetNumBounds() == 0)
		treeNodeFlags |= SlGuiTreeNodeFlags_NoChildren;
	else
		treeNodeFlags |= SlGuiTreeNodeFlags_DefaultOpen;

	// Draw node and handle selection
	bool selected = m_SelectedBound == bound;
	bool toggled;
	bool wasSelected = selected;
	bool treeOpened = SlGui::GraphTreeNode(treeNodeName, selected, toggled, treeNodeFlags);
	if (wasSelected != selected) // Selection changed
	{
		m_SelectedBound = bound;
		m_SelectedBoundCompositeIndex = compositeIndex;
		m_SelectedBoundComposite = compositeParent;

		// Depth is computed every call so cache it on selection change
		rage::phOptimizedBvh* octree = bound->GetBVH();
		m_SelectedBoundBVHDepth = octree ? octree->ComputeDepth() : 0;
		m_SelectedBoundBVHDepthDraw = -1;
	}

	// Draw bounding box for selected bound
	if (ImGui::IsItemHovered())
	{
		DrawList& dl = GameDrawLists::GetInstance()->Overlay;

		DrawList::SetTransform(transform);
		dl.DrawAABB(bound->GetBoundingBox(), graphics::COLOR_PINK);
		DrawList::ResetTransform();
	}

	// Draw child composite bounds
	if (treeOpened)
	{
		AM_ASSERTS(composite);
		for (u16 i = 0; i < composite->GetNumBounds(); i++)
		{
			// TODO: It's 34 in bound
			Mat44V childTransform = Mat34V(composite->GetMatrix(i)).To44() * transform;
			DrawBoundRecurse(composite->GetBound(i).Get(), childTransform, composite, i);
		}
		ImGui::TreePop();
	}

	// TODO:
	// ==
	// Display all polyhedron bound primitives!
}

void rageam::integration::ModelInspector::DrawBoundInfo()
{
	rage::phBoundType boundType = m_SelectedBound->GetShapeType();
	rage::phOptimizedBvh* octree = m_SelectedBound->GetBVH();
	const rage::spdAABB& bb = m_SelectedBound->GetBoundingBox();
	const rage::spdSphere& bs = m_SelectedBound->GetBoundingSphere();
	const rage::Vec3V& centroidOffset = m_SelectedBound->GetCentroidOffset();
	const rage::Vec3V& cgOffset = m_SelectedBound->GetCGOffset();

	ImGui::Text("Type: %s", rage::phBoundTypeName[boundType]);
	ImGui::Text("Primitive Material ID: %X", m_SelectedBound->GetPrimitiveMaterialId());
	ImGui::Text("AABB Min: <%.02f %.02f %.02f>", bb.Min.X(), bb.Min.Y(), bb.Min.Z());
	ImGui::Text("AABB Max: <%.02f %.02f %.02f>", bb.Max.X(), bb.Max.Y(), bb.Max.Z());
	ImGui::Text("Cull Sphere Center: <%.02f %.02f %.02f>", bs.CenterAndRadius.X(), bs.CenterAndRadius.Y(), bs.CenterAndRadius.Z());
	ImGui::Text("Cull Sphere Radius: %.02f", bs.CenterAndRadius.W());
	ImGui::Text("Centroid Offset: <%.02f %.02f %.02f>", centroidOffset.X(), centroidOffset.Y(), centroidOffset.Z());
	ImGui::Text("Center Of Gravity Offset: <%.02f %.02f %.02f>", cgOffset.X(), cgOffset.Y(), cgOffset.Z());

	if (boundType == rage::PH_BOUND_COMPOSITE)
	{
		SlGui::CategoryText("Composite");
		rage::phBoundComposite* composite = reinterpret_cast<rage::phBoundComposite*>(m_SelectedBound);
		ImGui::Text("Bound Count: %i", composite->GetNumBounds());
	}

	if (boundType == rage::PH_BOUND_GEOMETRY)
	{
		SlGui::CategoryText("Geometry");
		rage::phBoundGeometry* geometry = reinterpret_cast<rage::phBoundGeometry*>(m_SelectedBound);
	}

	// Info about this bound in a composite
	if (m_SelectedBoundComposite)
	{
		ImGui::HelpMarker(
			"Composite properties section shows information about this bound in the parent composite.\n"
			"For example, primitive bounds can't have any kind of transformation on their own, but composite bound allows to define a matrix.");
		ImGui::SameLine();
		SlGui::CategoryText("In-Composite Properties");

		u32 typeFlags = m_SelectedBoundComposite->GetTypeFlags(m_SelectedBoundCompositeIndex);
		u32 includeFlags = m_SelectedBoundComposite->GetIncludeFlags(m_SelectedBoundCompositeIndex);
		// TODO: VFT is unaligned
		u64 materialId = 0; // m_SelectedBoundComposite->GetMaterialId(m_SelectedBoundCompositeIndex);

		ImGui::Text("Type Flags: %X", typeFlags);
		ImGui::Text("Include Flags: %X", includeFlags);
		ImGui::Text("Material ID: %X", materialId);

		// Transformation
		const Mat44V& matrix = m_SelectedBoundComposite->GetMatrix(m_SelectedBoundCompositeIndex);
		Vec3V         pos, scale;
		QuatV         rot;
		matrix.Decompose(&pos, &scale, &rot);
		// Vec3V rotEuler = rot.ToEuler(); // TODO: ...
		SlGui::CategoryText("Transformation");
		ImGui::BeginDisabled();
		ImGui::DragFloat3("Position", pos.M.m128_f32);
		ImGui::DragFloat3("Scale", scale.M.m128_f32);
		ImGui::DragFloat4("Rotation", rot.M.m128_f32);
		ImGui::EndDisabled();
	}

	// BVH Octree
	if (boundType == rage::PH_BOUND_BVH || boundType == rage::PH_BOUND_COMPOSITE)
	{
		SlGui::CategoryText("Bounding Volume Hierarchy");
		if (!octree)
		{
			ImGui::TextDisabled("Octree was not generated for this bound.");
		}
		else
		{
			if (ImGui::Button("Print BVH"))
				octree->PrintInfo();
			ImGui::ToolTip("Writes BVH internal state and node hierarchy to the console.");

			ImGui::SetNextItemWidth(GImGui->FontSize * 5); // Slider is way too wide
			ImGui::SliderInt("Visualize Depth", &m_SelectedBoundBVHDepthDraw, -1, m_SelectedBoundBVHDepth - 1);
			ImGui::ToolTip("Draws bounding box of BVH nodes at selected depth");

			if (m_SelectedBoundBVHDepthDraw != -1)
			{
				octree->IterateTree([&](rage::phOptimizedBvhNode& node, int depth)
					{
						if (m_SelectedBoundBVHDepthDraw != depth)
							return;

						DrawList& dl = GameDrawLists::GetInstance()->Overlay;
						Mat44V boundOffset = Mat44V::Translation(m_SelectedBound->GetCentroidOffset());
						// TODO: This doesn't account composite transform...
						DrawList::SetTransform(boundOffset * GetEntity()->GetWorldTransform());
						dl.DrawAABB(octree->GetNodeAABB(node), graphics::COLOR_RED);
						DrawList::ResetTransform();
					});
			}
		}
	}
}

void rageam::integration::ModelInspector::DrawBounds()
{
	const rage::phBoundPtr& bound = m_Drawable->GetBound();

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
	DrawBoundRecurse(bound.Get(), GetEntity()->GetWorldTransform());
	ImGui::PopStyleVar();

	// TODO: Display bound index in composite for navigation

	if (m_SelectedBound)
	{
		// ImGui::DockBuilderAddNode(0, ImGuiDockNodeFlags_)
		// ImGui::DockBuilderSplitNode()
		if (ImGui::Begin("Bound Inspector"))
		{
			DrawBoundInfo();
		}
		ImGui::End();
	}
}

void rageam::integration::ModelInspector::DrawGeometry(rage::grmModel* model, u16 index)
{
	auto& geometry = model->GetGeometries().Get(index);

	if (!BeginCategory(ImGui::FormatTemp("Geometry #%i", index)))
		return;

	PropertyValue("Material Index", "%u", model->GetMaterialIndex(index));
	if (IconNav())
	{
		AM_UNREACHABLE("Material selection is not implemented.");
		// m_MaterialIndexToSelect = model->GetMaterialIndex(index);
	}
	
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
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	ImGuiStyle& style = ImGui::GetStyle();

	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(6, 2));
	bool propertiesTable = ImGui::BeginTable("Materials", 2, ImGuiTableFlags_SizingFixedFit);
	ImGui::PopStyleVar(); // ItemInnerSpacing
	if (!propertiesTable)
		return;

	ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed);
	ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
	for (u16 i = 0; i < shader->GetVarCount(); i++)
	{
		rage::grcEffectVar* varInfo = effect->GetVar(i);
		rage::grcInstanceVar* var = shader->GetVar(i);

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

		// Column: Name
		ImGui::TableNextColumn();
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.85f); // Grayed-out text helps to see categories and values better
		ImGui::Text(varInfo->GetName());
		ImGui::PopStyleVar();
		// Column: Value
		ImGui::TableNextColumn();
		{
			if (var->IsTexture())
			{
				rage::grcTexture* texture = var->GetTexture();
				int width, height;
				graphics::ImageFitInRect(texture->GetWidth(), texture->GetHeight(), 64, width, height);
				ImGui::ScrollingLabel(texture->GetName());
				ImGui::ToolTip(texture->GetName()); // Show full texture name on hover
				ImGui::Image(texture->GetTextureView(), ImVec2(static_cast<float>(width), static_cast<float>(height)));
			}
			else
			{
				// Render value text inside a rect, similar to what DragFloat/DragInt looks like
				ImRect rect(
					window->DC.CursorPos, 
					window->DC.CursorPos + ImVec2(window->WorkRect.GetWidth(), GImGui->FontSize + style.FramePadding.y * 2.0f));
				ImGui::ItemSize(rect, style.FramePadding.y);
				ImGui::ItemAdd(rect, 0);
				ImGui::RenderFrame(rect.Min, rect.Max, ImGui::GetColorU32(ImGuiCol_FrameBg));
				ImGui::RenderText(rect.Min + style.FramePadding, varValue);
			}
		}
	}

	ImGui::EndTable();
}

void rageam::integration::ModelInspector::DrawMaterialGroup()
{
	rage::grmShaderGroup* shaderGroup = m_Drawable->GetShaderGroup();
	if (shaderGroup->GetShaderCount() == 0)
	{
		ImGui::TextCentered("No materials...", ImGuiTextCenteredFlags_Horizontal);
		return;
	}

	// Material List
	ImGui::Text("Material:");
	ImGui::SetNextItemWidth(-1);
	int hoveredMaterial = -1;
	bool openedCombo = false;
	if (ImGui::BeginCombo("###MaterialPicker", FormatMaterialName(m_SelectedMaterial)))
	{
		for (u16 i = 0; i < shaderGroup->GetShaderCount(); i++)
		{
			ConstString shaderName = FormatMaterialName(i);

			bool selected = m_SelectedMaterial == i;
			if (ImGui::Selectable(shaderName, selected))
			{
				m_SelectedMaterial = i;
			}
			else
			{
				if (ImGui::IsItemHovered())
					hoveredMaterial = i;
			}
		}
		ImGui::EndCombo();
		openedCombo = true;
	}

	// Set outline to selected material
	if (openedCombo)
	{
		constexpr int outlineFlag = 1 << rage::RB_MODEL_OUTLINE;
		for (u16 i = 0; i < shaderGroup->GetShaderCount(); i++)
		{
			rage::grmShader* shader = shaderGroup->GetShader(i);
			// Reset all materials visibility flag
			if (hoveredMaterial == -1 || hoveredMaterial != i)
			{
				shader->SetDrawBucketMask(shader->GetDrawBucketMask() & ~outlineFlag);
			}
			else
			{
				shader->SetDrawBucketMask(shader->GetDrawBucketMask() | outlineFlag);
			}
		}
	}

	// Duplicated from MaterialEditor::DrawMaterialOptions
	static constexpr ConstString s_BucketNames[] =
	{
		"0 - Opaque",
		"1 - Alpha",
		"2 - Decal",
		"3 - Cutout",
		"4 - No Splash",
		"5 - No Water",
		"6 - Water",
		"7 - Displacement Alpha",
	};

	rage::grmShader* shader = shaderGroup->GetShader(m_SelectedMaterial);
	rage::grcEffect* effect = shader->GetEffect();
	ImGui::Indent();
	ConstString preset = shader->GetPresetName();
	if (preset)
		ImGui::Text("Preset: %s", preset);
	else
		ImGui::Text("Shader: %s.fxc", effect->GetName());
	ImGui::Text("Draw Bucket: %s, Mask: %X", s_BucketNames[shader->GetDrawBucket()], shader->GetDrawBucketMask());
	ImGui::Unindent();
	SlGui::CategoryText("Variables");
	DrawShaderParams(shader, effect);
}

void rageam::integration::ModelInspector::OnRender()
{
	// In case if drawable failed to load...
	if (!m_Drawable)
		return;

	if (ImGui::BeginTabBar("ModelInspector_TabBar"))
	{
		if (ImGui::BeginTabItem("Info"))
		{
			if (BeginProperties())
			{
				PropertyValue("Name", m_Drawable->GetName());
				PropertyValue("File Size", FormatSize(m_FileSize));
				ImGui::HelpMarker("Size of the resource file on disk.");
				PropertyValue("Resource Size", FormatSize(m_ResourceSize));
				ImGui::HelpMarker("Size taken by streamed resource in-game");
				ImGui::Indent();
				PropertyValue("Virtual", FormatSize(m_VirtualSize));
				PropertyValue("Physical", FormatSize(m_PhysicalSize));
				ImGui::Unindent();

				const rage::spdAABB& bb = m_Drawable->GetBoundingBox();
				const rage::spdSphere& bs = m_Drawable->GetBoundingSphere();
				PropertyValue("Box Min", FormatVec(bb.Min));
				PropertyValue("Box Max", FormatVec(bb.Max));
				PropertyValue("Sphere Center", FormatVec(bs.GetCenter()));
				PropertyValue("Sphere Radius", "%.02f", bs.GetRadius().Get());

				EndProperties();
			}
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Materials"))
		{
			DrawMaterialGroup();
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Lods"))
		{
			if (BeginProperties())
			{
				DrawLodGroup();
				EndProperties();
			}
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Bounds"))
		{
			DrawBounds();
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}
}

void rageam::integration::ModelInspector::LoadFromPath(ConstWString path)
{
	AM_ASSERTS(file::MatchExtension(path, L"ydr"));

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
		"E8 ?? ?? ?? ?? 48 01 83 B0 00 00 00 66 3B BB B8 00 00 00", "gtaDrawable::gtaDrawable(const datResource& rsc)+0x47").GetAt(-0x35)
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
	m_ArchetypeDef->Flags = FLAG_IS_TYPE_OBJECT | FLAG_IS_FIXED | FLAG_HAS_ANIM;
	// Add bounding sphere radius as long distance to draw it far enough
	m_ArchetypeDef->LodDist = m_Drawable->GetBoundingSphere().GetRadius().Get() + 200.0f;

	CreateEntity(m_Drawable, m_ArchetypeDef);
	GetEntity()->SetEntityWasAllocatedByGame(true); // Ensure that drawable will be destroyed with game allocator in TLS
}

#endif
