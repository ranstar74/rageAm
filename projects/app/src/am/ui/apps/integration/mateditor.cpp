#include "mateditor.h"

#include "am/algorithms/fuzzysearch.h"
#include "am/string/splitter.h"

#ifdef AM_INTEGRATED

#include "am/graphics/texture/imagefit.h"
#include "am/integration/hooks/streaming.h"
#include "am/ui/font_icons/icons_am.h"
#include "rage/atl/datahash.h"
#include "rage/streaming/assetstore.h"
#include "am/xml/doc.h"
#include "widgets.h"
#include "am/ui/styled/slgui.h"
#include "am/ui/styled/slwidgets.h"
#include "am/xml/iterator.h"

using fwTxdStore = rage::fwAssetStore<rage::grcTextureDictionary, rage::fwAssetDef<rage::grcTextureDictionary>>;
static fwTxdStore* GetTxdStore()
{
	static fwTxdStore* txdStore = static_cast<fwTxdStore*>(hooks::Streaming::GetModule("ytd"));
	return txdStore;
}

void rageam::integration::ShaderUIConfig::Load()
{
	CleanUp();

	try
	{
		XmlDoc xDoc;
		xDoc.LoadFromFile(GetFilePath());

		XmlHandle xShaderConfig = xDoc.Root();

		// Vars
		XmlHandle xUIVars = xShaderConfig.GetChild("UIVars", true);
		Vars.InitAndAllocate(xUIVars.GetChildCount("Item"), false);
		for (const XmlHandle& item : XmlIterator(xUIVars, "Item"))
		{
			// Parse main properties
			ConstString varName;
			ConstString overrideName;
			ConstString description;
			item.GetAttribute("Name", varName);
			item.GetChildText("Name", &overrideName);
			item.GetChildText("Description", &description);

			UIVar uiVar;
			uiVar.NameHash = Hash(varName);
			uiVar.OverrideName = overrideName;
			uiVar.Description = description;

			// Check if user added more than one config for one UI var
			if (Vars.ContainsAt(uiVar.NameHash))
			{
				AM_ERRF("ShaderConfig::Load() -> Variable '%s' already was added! Check config for duplicates.", varName);
				CleanUp();
				return;
			}

			// Parse widget
			XmlHandle xWidget = item.GetChild("Widget");
			if (!xWidget.IsNull())
			{
				xWidget.GetAttribute("Type", uiVar.WidgetType);

				switch (uiVar.WidgetType)
				{
				case SliderFloat:
				case SliderFloatLogarithmic:
					xWidget.GetChild("Min").GetValue(uiVar.WidgetParams.SliderFloat.Min);
					xWidget.GetChild("Max").GetValue(uiVar.WidgetParams.SliderFloat.Max);
					break;

				case ToggleFloat:
					xWidget.GetChild("Enabled").GetValue(uiVar.WidgetParams.ToggleFloat.Enabled);
					xWidget.GetChild("Disabled").GetValue(uiVar.WidgetParams.ToggleFloat.Disabled);
					break;

				default:
					break;
				}
			}
			Vars.EmplaceAt(uiVar.NameHash, std::move(uiVar));
		}
	}
	catch (const XmlException& ex)
	{
		ex.Print();
		AM_ERRF("ShaderConfig::Load() -> Failed to load ShaderUI.xml");
		CleanUp();
	}
}

void rageam::integration::ShaderUIConfig::CleanUp()
{
	Vars.Destruct();
}

rageam::integration::ShaderUIConfig::UIVar* rageam::integration::ShaderUIConfig::GetUIVarFor(const rage::grcEffectVar* varInfo) const
{
	UIVar* uiVar;

	// Try to retrieve from name / display name hashes
	uiVar = Vars.TryGetAt(varInfo->GetNameHash());
	if (uiVar) return uiVar;
	uiVar = Vars.TryGetAt(varInfo->GetDisplayNameHash());
	return uiVar;
}

void rageam::integration::MaterialEditor::InitializePresetSearch()
{
	auto findShaderPreset = gmAddress::Scan("48 89 5C 24 08 4C 8B 05").ToFunc<rage::grcInstanceData * (u32)>();

	rage::fiStreamPtr preloadList = rage::fiStream::Open("common:/shaders/db/preload.list");
	if (!preloadList)
		return;
	char fileNameBuffer[64];
	while (preloadList->ReadLine(fileNameBuffer, sizeof fileNameBuffer))
	{
		u32 fileNameHash = rage::joaat(fileNameBuffer);

		rage::grcInstanceData* instanceData = findShaderPreset(fileNameHash);
		if (!instanceData)
		{
			AM_WARNINGF("MaterialEditor::InitializePresetSearch() -> Unable to find preset '%s'", fileNameBuffer);
			continue;
		}

		rage::grcEffect* effect = instanceData->GetEffect();
		// Check if shader can be used for drawables
		if (effect->LookupTechnique(TECHNIQUE_DRAW) == INVALID_FX_HANDLE)
			continue;

		ShaderPreset shaderPreset;
		shaderPreset.InstanceData = instanceData;
		shaderPreset.FileName = fileNameBuffer;
		shaderPreset.FileNameHash = fileNameHash;

		// Trim '.sps'
		string name = fileNameBuffer;
		name = name.Substring(0, name.IndexOf<'.'>());
		shaderPreset.Name = std::move(name);

		ComputePresetFilterTagAndTokens(shaderPreset);

		m_ShaderPresets.Emplace(std::move(shaderPreset));
	}
	m_SearchIndices.Reserve(m_ShaderPresets.GetSize());
}

void rageam::integration::MaterialEditor::ComputePresetFilterTagAndTokens(ShaderPreset& preset) const
{
	preset.TokenCount = 0;

	const string& name = preset.Name;

	u32 c = 0; // Categories
	u32 m = 0; // Maps

	// Tokenize 'normal_spec_decal' on 'normal 'spec' 'decal' and assign tags
	size_t tokenOffset, tokenLength;
	ConstString token;
	StringSplitter<'_'> splitter(name);
	while (splitter.GetNext(token, &tokenOffset, &tokenLength))
	{
		u32 tokenHash = Hash(token);

		if (preset.TokenCount >= SHADER_MAX_TOKENS)
		{
			AM_WARNINGF("MaterialEditor::GetShaderTagAndTokens() -> Too much tokens in '%s'", name.GetCStr());
		}
		else
		{
			preset.Tokens[preset.TokenCount++] = std::string_view(name + tokenOffset, tokenLength);
		}

		switch (tokenHash)
		{
			// Categories
		case Hash("vehicle"):		c |= ST_Vehicle;	break;
		case Hash("ped"):			c |= ST_Ped;		break;
		case Hash("weapon"):		c |= ST_Weapon;		break;
		case Hash("terrain"):		c |= ST_Terrain;	break;
		case Hash("glass"):			c |= ST_Glass;		break;
		case Hash("decal"):			c |= ST_Decal;		break;
		case Hash("cloth"):			c |= ST_Cloth;		break;
		case Hash("cutout"):		c |= ST_Cutout;		break;
		case Hash("emissivenight"):
		case Hash("emissive"):		c |= ST_Emissive;	break;
		case Hash("pxm"):
		case Hash("parallax"):		c |= ST_Parallax;	break;
			// Maps
		case Hash("detail"):
		case Hash("detail2"):		m |= SM_Detail;		break;
		case Hash("spec"):
		case Hash("specmap"):		m |= SM_Specular;	break;
		case Hash("normal"):		m |= SM_Normal;		break;
		case Hash("enveff"):
		case Hash("reflect"):		m |= SM_Cubemap;	break;
		case Hash("tnt"):			m |= SM_Tint;		break;
		}
	}

	if (c == 0)
		c |= ST_Misc; // Not in any of main categories, throw into misc

	preset.Tag.Categories = c;
	preset.Tag.Maps = m;
}

rage::grmShader* rageam::integration::MaterialEditor::GetSelectedMaterial() const
{
	return m_Drawable->GetShaderGroup()->GetShader(m_SelectedMaterialIndex);
}

rage::grcTexture* rageam::integration::MaterialEditor::TexturePicker(ConstString id, const rage::grcTexture* currentTexture)
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

void rageam::integration::MaterialEditor::HandleMaterialValueChange(u16 varIndex, rage::grcInstanceVar* var, const rage::grcEffectVar* varInfo) const
{
	// Toggle tessellation
	if (varInfo->GetNameHash() == rage::joaat("useTessellation"))
	{
		m_Drawable->ComputeTessellationForShader(m_SelectedMaterialIndex);
	}
}

void rageam::integration::MaterialEditor::HandleMaterialSelectionChanged()
{
	//// Clear shader preset search
	//m_ShaderPresetsSearch.Clear();
	//m_ShaderPresetSearchText[0] = '\0';
}

void rageam::integration::MaterialEditor::SearchTexture(ImmutableString searchText)
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

void rageam::integration::MaterialEditor::DoFuzzySearch()
{
	// Timer searchTimer = Timer::StartNew();

	m_SearchIndices.Clear();

	// Tokenize search and store in array
	List<string> searchTokens;
	{
		ConstString tokenTemp;
		StringSplitter<' ', '_', '-', ',', ';'> splitter(m_SearchText);
		splitter.SetTrimSpaces(true);
		while (splitter.GetNext(tokenTemp))
		{
			string token = tokenTemp;
			token = token.ToLowercase(); // All presets are lower case

			searchTokens.Emplace(std::move(token));
		}
	}

	List<float> presetDistances;
	presetDistances.Reserve(m_ShaderPresets.GetSize());

	for (u16 i = 0; i < m_ShaderPresets.GetSize(); i++)
	{
		ShaderPreset& preset = m_ShaderPresets[i];

		float totalDistance = 0.0f; // More = worse match

		// We introduce 'super kill' system where more good matches increases changes of preset coming first
		// Otherwise we may get case when 'vehicle_mesh' comes before 'vehicle_paint1_enveff'
		// with search 'vehicle, paint'
		float matchMultiplier = 1.0f;

		// Compare each preset token against search token
		for (u32 k = 0; k < preset.TokenCount; k++)
		{
			std::string_view& presetTokenView = preset.Tokens[k];
			// Copy token to local buffer, fuzzy compare doesn't work with string view
			char presetToken[128]{};
			presetTokenView.copy(presetToken, presetTokenView.length());

			// Find distance of best matching token
			float distance = FLT_MAX;
			for (string& searchToken : searchTokens)
			{
				// First do straight search, then do fuzzy
				ImmutableString lhs = presetToken;
				if (lhs.StartsWith(searchToken))
					distance = 0.0f; //
				else
					distance = MIN(distance, static_cast<float>(LevenshteinDistance(presetToken, searchToken)));
			}

			if (distance <= 2.0f) matchMultiplier += 0.5f;
			if (distance == 0.0f) matchMultiplier += 1.25f;

			distance /= matchMultiplier;

			// Normalize by total token count to prevent bias towards presets with less tokens
			totalDistance += distance /= static_cast<float>(preset.TokenCount);
		}

		// No match
		if (totalDistance > 3.0f)
			continue;

		// AM_DEBUGF("%s (D:%g)", preset.Name.GetCStr(), totalDistance);

		// Find where current search result goes in leader board
		u16 insertIndex = 0;
		for (; insertIndex < presetDistances.GetSize(); insertIndex++)
		{
			if (totalDistance <= presetDistances[insertIndex])
			{
				break;
			}
		}

		presetDistances.Insert(insertIndex, totalDistance);
		m_SearchIndices.Insert(insertIndex, i);
	}

	// searchTimer.Stop();
	// AM_DEBUGF(":MaterialEditor::DrawShaderList() -> Search took %gs", searchTimer.GetElapsedMicroseconds() / 1000000.0);
}

void rageam::integration::MaterialEditor::DrawShaderSearchListItem(u16 index)
{
	ImVec2 buttonSize(ImGui::GetContentRegionAvail().x, 0);
	graphics::ColorU32 buttonColor = ImGui::GetColorU32(ImGuiCol_Button);
	buttonColor.A = 25;

	ShaderPreset& preset = m_ShaderPresets[index];

	// Only presets that include all selected categories
	if (m_SearchCategories != 0 && (preset.Tag.Categories | m_SearchCategories) != preset.Tag.Categories)
		return;

	// Only presets that include all selected maps
	if (m_SearchMaps != 0 && (preset.Tag.Maps | m_SearchMaps) != preset.Tag.Maps)
		return;

	// TODO: Show selected shader

	SlGui::PushFont(SlFont_Small);
	ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
	ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f)); // Align left
	if (ImGui::ButtonEx(preset.Name, buttonSize))
	{
		// Clone material from preset
		rage::grmShader* material = m_Drawable->GetShaderGroup()->GetShader(m_SelectedMaterialIndex);
		material->CloneFrom(*preset.InstanceData);
	}
	ImGui::PopStyleVar();	// ButtonTextAlign
	ImGui::PopStyleColor(); // Button
	SlGui::PopFont();
}

void rageam::integration::MaterialEditor::DrawShaderSearchList()
{
	// Search box
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
	if (ImGui::InputText("###SEARCH", m_SearchText, sizeof m_SearchText))
		DoFuzzySearch();
	ImGui::InputTextPlaceholder(m_SearchText, "Search...");

	bool hasSearch = m_SearchText[0] != '\0';

	// Reserve space for status bar
	float height = ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeight();

	// Shaders
	ImGui::BeginChild("SHADER_LIST", ImVec2(0, height));
	{
		SlGui::ShadeItem(SlGuiCol_Bg);

		// Display either search result or all presets
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 1));
		if (hasSearch)
		{
			for (u16 i : m_SearchIndices)
				DrawShaderSearchListItem(i);
		}
		else
		{
			for (u16 i = 0; i < m_ShaderPresets.GetSize(); i++)
				DrawShaderSearchListItem(i);
		}
		ImGui::PopStyleVar(); // ItemSpacing
	}
	ImGui::EndChild();

	// Count on bottom of window
	u16 itemCount = hasSearch ? m_SearchIndices.GetSize() : m_ShaderPresets.GetSize();
	ImGui::Dummy(ImVec2(4, 4)); ImGui::SameLine(0, 0); // Window has no padding, we have to add it manually
	ImGui::Text("%u Item(s)", itemCount);
}

void rageam::integration::MaterialEditor::DrawShaderSearchFilters()
{
#define CATEGORY_FLAG(name) ImGui::CheckboxFlags(#name, &m_SearchCategories, ST_ ##name);
#define MAP_FLAG(name) ImGui::CheckboxFlags(#name, &m_SearchMaps, SM_ ##name);

	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(1, 1));

	// ShaderCategories
	SlGui::CategoryText("Categories");
	SlGui::PushFont(SlFont_Small);
	if (ImGui::BeginTable("SHADER_CATEGORIES_TABLE", 2))
	{
		ImGui::TableNextColumn(); CATEGORY_FLAG(Vehicle);	ImGui::TableNextColumn(); CATEGORY_FLAG(Ped);
		ImGui::TableNextColumn(); CATEGORY_FLAG(Weapon);	ImGui::TableNextColumn(); CATEGORY_FLAG(Terrain);
		ImGui::TableNextColumn(); CATEGORY_FLAG(Glass);		ImGui::TableNextColumn(); CATEGORY_FLAG(Decal);
		ImGui::TableNextColumn(); CATEGORY_FLAG(Cloth);		ImGui::TableNextColumn(); CATEGORY_FLAG(Cutout);
		ImGui::TableNextColumn(); CATEGORY_FLAG(Emissive);	ImGui::TableNextColumn(); CATEGORY_FLAG(Parallax);
		ImGui::TableNextColumn(); CATEGORY_FLAG(Misc);
		ImGui::EndTable();
	}
	SlGui::PopFont();

	// ShaderMaps
	SlGui::CategoryText("Maps");
	SlGui::PushFont(SlFont_Small);
	if (ImGui::BeginTable("SHADER_MAPS_TABLE", 1))
	{
		ImGui::TableNextColumn(); MAP_FLAG(Detail);
		ImGui::TableNextColumn(); MAP_FLAG(Specular);
		ImGui::TableNextColumn(); MAP_FLAG(Normal);
		ImGui::TableNextColumn(); MAP_FLAG(Cubemap);
		ImGui::TableNextColumn(); MAP_FLAG(Tint);
		ImGui::EndTable();
	}
	SlGui::PopFont();

	ImGui::PopStyleVar(1); // CellPadding

#undef CATEGORY_FLAG
#undef MAP_FLAG
}

void rageam::integration::MaterialEditor::DrawShaderSearch()
{

	if (ImGui::BeginTable("SHADER_LIST_TABLE", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV))
	{
		ImGui::TableNextRow();

		// Column: List
		if (ImGui::TableNextColumn())
		{
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
			DrawShaderSearchList();
			ImGui::PopStyleVar();
		}

		// Column: Filters
		if (ImGui::TableNextColumn())
		{
			SlGui::BeginPadded("SHADER_SEARCH_FILTERS_PADDING", ImVec2(4, 4));
			DrawShaderSearchFilters();
			SlGui::EndPadded();
		}

		ImGui::EndTable();
	}
}

void rageam::integration::MaterialEditor::DrawMaterialList()
{
	// TODO: Sphere Ball Preview & Preview resizing (ItemDragged?)

	if (!ImGui::BeginChild("MaterialEditor_List"))
	{
		ImGui::EndChild();
		return;
	}
	SlGui::ShadeItem(SlGuiCol_Bg);

	rage::grmShaderGroup* shaderGroup = m_Drawable->GetShaderGroup();

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
	for (u16 i = 0; i < shaderGroup->GetShaderCount(); i++)
	{
		rage::grmShader* shader = shaderGroup->GetShader(i);
		rage::grcEffect* effect = shader->GetEffect();

		// TODO: Configuration what to use as material name if no map present
		ConstString materialName = effect->GetName();

		// Retrieve material name from scene
		if (m_DrawableMap)
		{
			u16 sceneMaterialIndex = m_DrawableMap->ShaderToSceneMaterial[i];
			if (sceneMaterialIndex == u16(-1)) // Invalid index is used only for implicitly created default material
			{
				materialName = "Default";
			}
			else
			{
				materialName = m_Scene->GetMaterial(sceneMaterialIndex)->GetName();
			}
		}

		ConstString nodeName = ImGui::FormatTemp("%s###%s_%i", materialName, materialName, i);

		bool selected = m_SelectedMaterialIndex == i;
		bool toggled;
		SlGui::GraphTreeNode(nodeName, selected, toggled, SlGuiTreeNodeFlags_NoChildren);
		if (selected && m_SelectedMaterialIndex != i)
		{
			m_SelectedMaterialIndex = i;
			HandleMaterialSelectionChanged();
		}
	}
	ImGui::PopStyleVar(); // Item_Spacing
	ImGui::EndChild();    // MaterialEditor_List
}

void rageam::integration::MaterialEditor::DrawMaterialVariables()
{
	if (!SlGui::BeginPadded("MaterialEditor_Properties_Padding", ImVec2(6, 6)))
	{
		SlGui::EndPadded();
		return;
	}

	rage::grmShader* material = GetSelectedMaterial();

	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(6, 2));
	bool propertiesTable = ImGui::BeginTable("MaterialEditor_Properties", 2, ImGuiTableFlags_SizingFixedFit);
	ImGui::PopStyleVar(); // ItemInnerSpacing
	if (!propertiesTable)
		return;

	ImGui::TableSetupColumn("MaterialEditor_Properties_Name", ImGuiTableColumnFlags_WidthFixed);
	ImGui::TableSetupColumn("MaterialEditor_Properties_Value", ImGuiTableColumnFlags_WidthStretch);

	rage::grcEffect* effect = material->GetEffect();
	for (u16 i = 0; i < material->GetVarCount(); i++)
	{
		ImGui::TableNextRow();

		// Material (shader in rage terms) holds variable values without any metadata,
		// we have to query effect variable to get name and type
		rage::grcInstanceVar* var = material->GetVarByIndex(i);
		rage::grcEffectVar* varInfo = effect->GetVarByIndex(i);

		ShaderUIConfig::UIVar* uiVar = m_UIConfig.GetUIVarFor(varInfo);

		// Column: Variable Name
		if (ImGui::TableNextColumn())
		{
			ConstString name = varInfo->GetName();

			if (uiVar)
			{
				// Retrieve override name from UI config
				if (!String::IsNullOrEmpty(uiVar->OverrideName))
				{
					name = uiVar->OverrideName;
				}

				// Add description from UI config
				if (!String::IsNullOrEmpty(uiVar->Description))
				{
					ImGui::HelpMarker(uiVar->Description);
					ImGui::SameLine(0, 2);
				}
			}

			ImGui::Text("%s", name);
		}

		// Column: Variable value
		if (ImGui::TableNextColumn())
		{
			ConstString inputID = ImGui::FormatTemp("###%s_%i", varInfo->GetName(), i);

			// Stretch input field
			ImGui::SetNextItemWidth(-1);

			// Keep track if variable value was actually changed
			bool edited = false;

			// Default widget is always drawn for texture variable, it can't be overriden
			bool useDefaultWidget =
				var->IsTexture() ||
				uiVar == nullptr ||
				uiVar->WidgetType == ShaderUIConfig::Default;

			if (useDefaultWidget)
			{
				switch (varInfo->GetType())
				{
				case rage::EFFECT_VALUE_INT:
				case rage::EFFECT_VALUE_INT1:		edited = ImGui::InputInt(inputID, var->GetValuePtr<int>());						break;
				case rage::EFFECT_VALUE_INT2:		edited = ImGui::InputInt2(inputID, var->GetValuePtr<int>());					break;
				case rage::EFFECT_VALUE_INT3:		edited = ImGui::InputInt3(inputID, var->GetValuePtr<int>());					break;
				case rage::EFFECT_VALUE_FLOAT:		edited = ImGui::DragFloat(inputID, var->GetValuePtr<float>(), 0.1f);			break;
				case rage::EFFECT_VALUE_VECTOR2:	edited = ImGui::DragFloat2(inputID, var->GetValuePtr<float>(), 0.1f);			break;
				case rage::EFFECT_VALUE_VECTOR3:	edited = ImGui::DragFloat3(inputID, var->GetValuePtr<float>(), 0.1f);			break;
				case rage::EFFECT_VALUE_VECTOR4:	edited = edited = ImGui::DragFloat4(inputID, var->GetValuePtr<float>(), 0.1f);	break;
				case rage::EFFECT_VALUE_BOOL:		edited = SlGui::Checkbox(inputID, var->GetValuePtr<bool>());					break;

				case rage::EFFECT_VALUE_TEXTURE:
				{
					rage::grcTexture* newTexture = TexturePicker(inputID, var->GetValuePtr<rage::grcTexture>());
					if (newTexture)
					{
						var->SetTexture(newTexture);
						edited = true;
					}
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
			else // Widget override
			{
				auto& sliderFloatParams = uiVar->WidgetParams.SliderFloat;
				auto& toggleFloatParams = uiVar->WidgetParams.ToggleFloat;
				switch (uiVar->WidgetType)
				{
				case ShaderUIConfig::SliderFloat:
				{
					edited = ImGui::SliderFloat(inputID, var->GetValuePtr<float>(), sliderFloatParams.Min, sliderFloatParams.Max);
					break;
				}
				case ShaderUIConfig::SliderFloatLogarithmic:
				{
					edited = ImGui::SliderFloat(
						inputID, var->GetValuePtr<float>(), sliderFloatParams.Min, sliderFloatParams.Max, "%.3f",
						ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);
					break;
				}
				case ShaderUIConfig::ToggleFloat:
				{
					bool isChecked = rage::Math::AlmostEquals(var->GetValue<float>(), toggleFloatParams.Enabled);
					if (SlGui::Checkbox(inputID, &isChecked))
					{
						var->SetValue<float>(isChecked ? toggleFloatParams.Enabled : toggleFloatParams.Disabled);
						edited = true;
					}
					break;
				}

				default:
					ImGui::TextColored(ImVec4(1, 0, 0, 1), "Widget %s is not implemented", Enum::GetName(uiVar->WidgetType));
					break;
				}
			}

			if (edited)
				HandleMaterialValueChange(i, var, varInfo);
		}
	}
	ImGui::EndTable();  // MaterialEditor_Properties
	SlGui::EndPadded(); // MaterialEditor_Properties_Padding
}

void rageam::integration::MaterialEditor::DrawMaterialOptions() const
{
	if (!SlGui::BeginPadded("MaterialEditor_Options_Padding", ImVec2(6, 6)))
	{
		SlGui::EndPadded();
		return;
	}

	rage::grmShader* material = GetSelectedMaterial();

	// Draw Bucket
	static constexpr ConstString s_BucketNames[] =
	{
		"0 - Diffuse",
		"1 - Alpha",
		"2 - Decal",
		"3 - Cutout, Shadows",
		"4 - No Splash",
		"5 - No Water",
		"6 - Water",
		"7 - Lens Distortion",
	};

	ImGui::Text("Draw Bucket");
	ImGui::SameLine();
	ImGui::HelpMarker(
		"Change only if necessary. Bucket will be automatically picked from shader preset (.sps)\n"
		"Bucket descriptions:\n"
		"0 - Solid objects (no alpha)\n"
		"1 - Alpha without shadows, commonly used on glass\n"
		"2 - Decals with alpha, no shadows\n"
		"3 - Cutout with alpha + shadows, used on fences.\n"
		"4 - Used only on single preset 'vehicle_nosplash.sps'.\n"
		"5 - Used only on single preset 'vehicle_nowater.sps'.\n"
		"6 - Water shaders\n"
		"7 - Lens distortion (rendered last to apply effect on all objects in scene) 'glass_displacement.sps'");

	int drawBucket = material->GetDrawBucket();
	bool drawBucketEdited = false;
	if (ImGui::AddSubButtons("DRAW_BUCKET_ADD_SUB", drawBucket, 0, 7, false))
		drawBucketEdited = true;
	ImGui::SameLine(0, 1);
	if (ImGui::Combo("###DRAW_BUCKET", &drawBucket, s_BucketNames, IM_ARRAYSIZE(s_BucketNames)))
		drawBucketEdited = true;

	if (drawBucketEdited)
	{
		material->SetDrawBucket(drawBucket);
		m_Drawable->ComputeBucketMask();
	}

	// Render Flags
	SlGui::CategoryText("Render Flags");
	rage::grcRenderMask& renderMask = material->GetDrawBucketMask();
	u32 renderFlags = renderMask.GetRenderFlags();
	if (widgets::EnumFlags<rage::grcRenderFlags>("RENDER_FLAGS", "LF", &renderFlags))
	{
		ImGui::CheckboxFlags("Visibility", &renderFlags, rage::RF_VISIBILITY);
		ImGui::CheckboxFlags("Shadows", &renderFlags, rage::RF_SHADOWS);
		ImGui::CheckboxFlags("Reflections", &renderFlags, rage::RF_REFLECTIONS);
	}
	renderMask.SetRenderFlags(renderFlags);

	SlGui::EndPadded();
}

rageam::integration::MaterialEditor::MaterialEditor()
{
	m_UIConfig.Load();
	m_UIConfigWatcher.SetEntry(file::PathConverter::WideToUtf8(m_UIConfig.GetFilePath()));
}

void rageam::integration::MaterialEditor::Render()
{
	if (!IsOpen || !m_Drawable)
		return;

	// Load presets
	if (!m_ShaderPresets.Any())
	{
		InitializePresetSearch();
	}

	bool isOpen = IsOpen;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	bool window = ImGui::Begin(ICON_AM_BALL" Material Editor", &isOpen/*, ImGuiWindowFlags_MenuBar*/);
	ImGui::PopStyleVar(1); // WindowPadding
	IsOpen = isOpen;

	if (window)
	{
		// Reload UI config if file was changed
		if (m_UIConfigWatcher.GetChangeOccuredAndReset())
		{
			m_UIConfig.Load();
		}

		//if (ImGui::BeginMenuBar())
		//{
		//	static bool s_ShowShaders = true;
		//	if (SlGui::ToggleButton("Show Shaders", s_ShowShaders))
		//	{

		//	}

		//	ImGui::EndMenuBar();
		//}

		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0, 0));
		if (ImGui::BeginTable("MaterialEditor_SplitView", 3, ImGuiTableFlags_Resizable))
		{
			float defaultListWidth = ImGui::GetTextLineHeight() * 10;
			ImGui::TableSetupColumn("MaterialEditor_ListAndPreview", ImGuiTableColumnFlags_WidthFixed, defaultListWidth);
			ImGui::TableSetupColumn("MaterialEditor_Properties", ImGuiTableColumnFlags_WidthStretch, 0);
			ImGui::TableSetupColumn("MaterialEditor_Shaders");

			// Column: Preview & Mat List
			if (ImGui::TableNextColumn())
			{
				DrawMaterialList();
			}

			// Column: Mat Properties
			if (ImGui::TableNextColumn())
			{
				SlGui::BeginPadded("MATERIAL_PROPERTIES_PADDING", ImVec2(4, 4));
				ImGui::Text("Shader: %s.fxc", GetSelectedMaterial()->GetEffect()->GetName());
				ImGui::SameLine();
				ImGui::HelpMarker(
					"Shader (.fxc) name, not preset (.sps)!\n"
					"Preset only contains shader and settings (for example draw bucket index).");
				if (ImGui::BeginTabBar("SHADERS_TAB_BAR"))
				{
					if (ImGui::BeginTabItem("Variables"))
					{
						DrawMaterialVariables();
						ImGui::EndTabItem();
					}

					if (ImGui::BeginTabItem("Options"))
					{
						DrawMaterialOptions();
						ImGui::EndTabItem();
					}

					ImGui::EndTabBar();
				}
				SlGui::EndPadded();
			}

			// Column: Shaders
			if (ImGui::TableNextColumn())
			{
				DrawShaderSearch();
			}

			ImGui::EndTable();
		}
		ImGui::PopStyleVar(1); // CellPadding
	}
	ImGui::End();
}

void rageam::integration::MaterialEditor::SetDrawable(rage::rmcDrawable* drawable)
{
	m_Drawable = drawable;
	m_SelectedMaterialIndex = 0;
}

void rageam::integration::MaterialEditor::SetMap(graphics::Scene* scene, asset::DrawableAssetMap* map)
{
	m_Scene = scene;
	m_DrawableMap = map;
}

#endif
