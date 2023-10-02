//
// File: drawable.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include <any>

#include "txd.h"
#include "am/types.h"
#include "am/asset/gameasset.h"
#include "am/graphics/scene.h"
#include "am/system/nullable.h"
#include "game/drawable.h"

namespace rageam::asset
{
	// Version History:
	// 0: Initial
	
	// TODO:
	// Auto color-texture map generation for models that don't have texture
	// but only single color, to make life easier

	// TODO:
	// Some kind of workspace for shared textures?

	// TODO:
	// Dummy bones

	// TODO:
	// We probably config for shader parameters,
	// it would make sense to have global map because they're mostly
	// shared between the shaders

	static constexpr u16 MAX_BONES = 128;
	static constexpr int MAX_LOD = 4;
	static constexpr ConstString DEFAULT_SHADER = "default"; // default.fxc
	static constexpr ConstWString PALETTE_TEXTURE_NAME = L"autogen_palette";
	static constexpr ConstWString EMBED_DICT_NAME = L"textures";
	static constexpr ConstString COL_MODEL_EXT = ".COL"; // Postfix of collision models in scene

	struct MaterialTune : IXml
	{
		XML_DEFINE(MaterialTune);

		enum eParamType
		{
			// Keep no prefix because we serialize names directly + this enum is
			// in placed Material namespace, won't really matter

			None,
			String,
			Bool,
			Float,
			Vec2F,
			Vec3F,
			Vec4F,
			Color, // For palette texture generation

			Unsupported = -1,
		};

		inline static constexpr eParamType grcEffectVarTypeToParamType[] =
		{
			None,			// NONE
			Unsupported,	// INT
			Float,			// FLOAT
			Vec2F,			// VECTOR2
			Vec3F,			// VECTOR3
			Vec4F,			// VECTOR4
			String,			// TEXTURE
			Bool,			// BOOL
			Unsupported,	// MATRIX34
			Unsupported,	// MATRIX44
			String,			// STRING
			Unsupported,	// INT1
			Unsupported,	// INT2
			Unsupported,	// INT3
			Unsupported,	// INT4
		};

		struct Param : IXml
		{
			XML_DEFINE(Param);

			string		Name;
			eParamType	Type = None;
			std::any	Value;

			void Serialize(XmlHandle& xml) const override;
			void Deserialize(const XmlHandle& xml) override;

			void CopyValue(rage::grcInstanceVar* var);

			template<typename T>
			T GetValue()
			{
				return std::any_cast<T>(Value);
			}
		};

		string		Shader;
		string		Name;
		u32			ShaderHash;
		u32			NameHash;
		List<Param>	Params;

		MaterialTune() = default;
		MaterialTune(ConstString name, ConstString shaderName = DEFAULT_SHADER)
		{
			Name = name;
			Shader = shaderName;

			ShaderHash = joaat(Shader);
			NameHash = joaat(Name);
		}

		void Deserialize(const XmlHandle& xml) override;
		void Serialize(XmlHandle& xml) const override;

		// Finds effect from ShaderHash
		rage::grcEffect* LookupEffect() const;
		// Attempts to look up effect and uses DEFAULT_SHADER if not found
		rage::grcEffect* LookupEffectOrUseDefault() const;

		// Resets current parameters and defaults them from specified shader
		void InitFromShader();
		// Attempts to match texture name from material to 'DiffuseSampler' or 'SpecSampler' effect params
		void SetTextureNamesFromSceneMaterial(const graphics::SceneMaterial* sceneMaterial);
	};

	struct MaterialTuneGroup : IXml
	{
		Nullable<MaterialTune> DefaultMaterial;
		// Material indices match to SceneMaterial index
		List<MaterialTune> Materials;

		MaterialTune& GetMaterial(const graphics::SceneMaterial* sceneMaterial)
		{
			return Materials[sceneMaterial->GetIndex()];
		}

		const MaterialTune& GetMaterial(const graphics::SceneMaterial* sceneMaterial) const
		{
			return Materials[sceneMaterial->GetIndex()];
		}

		void Serialize(XmlHandle& node) const override;
		void Deserialize(const XmlHandle& node) override;
	};

	struct ModelTune
	{
		bool UseAsBound = false; // Uses least detailed lod as bound
	};
	using ModelTunes = List<ModelTune>;

	struct LodGroupTune : IXml
	{
		string	FileName;

		//Models	Models[MAX_LOD]{};
		float	LodThreshold[MAX_LOD] = { 30, 60, 90, 120 };
		bool	AutoGenerateLods = false; // TODO: We need some 3D mesh processing library

		void Serialize(XmlHandle& node) const override;
		void Deserialize(const XmlHandle& node) override;
	};

	struct LightTune
	{

	};
	using Lights = List<LightTune>;

	struct DrawableTune : AssetSource
	{
		MaterialTuneGroup	MaterialTuneGroup;
		LodGroupTune		LodGroup;
		Lights				Lights;
		bool				ForceGenerateSkeleton = false; // Bone will be created for every model (mesh)

		DrawableTune(AssetBase* parent, ConstWString fileName) : AssetSource(parent, fileName)
		{

		}

		void Serialize(XmlHandle& node) const override;
		void Deserialize(const XmlHandle& node) override;
	};

	class DrawableAsset : public GameRscAsset<gtaDrawable>
	{
		// Path's used only if loaded from xml

		file::WPath			m_ScenePath;
		file::WPath			m_EmbedDictPath; // By default (e.g. for 'bender.idr') will be 'bender.idr/bender.itd'

		TxdAssetPtr			m_EmbedDictTune;
		graphics::ScenePtr	m_Scene;
		DrawableTune		m_DrawableTune;

		// Fields below used only during conversion

		struct SplittedGeometry
		{
			rage::pgUPtr<rage::grmGeometryQB>	GrmGeometry;
			rage::spdAABB						BoundingBox;
		};

		struct EffectInfo
		{
			rage::grcEffectPtr			Effect;
			graphics::VertexDeclaration VertexDecl;		// VS_Transform
			graphics::VertexDeclaration VertexDeclSkin;	// VS_TransformSkin

			EffectInfo(rage::grcEffect* effect)
			{
				Effect = effect;
				VertexDecl.FromEffect(effect, false);
				VertexDeclSkin.FromEffect(effect, true);
			}
		};

		gtaDrawable* m_Drawable = nullptr;

		// NOTE: May be null if drawable has no embed textures
		// No smart pointer here because pointer ownership will be passed to drawable
		rage::grcTextureDictionary* m_EmbedDict = nullptr;
		// We cache reflected vertex declaration & fvf from effect here (key is effect name without .fxc extension)
		HashSet<EffectInfo> m_EffectCache;
		// Key is SceneNode index
		List<rage::grmModel*> m_NodeToModel;
		List<rage::crBoneData*> m_NodeToBone;

		// Looks up effect (shader) for every material and caches them in m_EffectCache
		bool CacheEffects();
		void PrepareForConversion();
		void CleanUpConversion();

		// Performs three things:
		// - Composes vertex buffer for given vertex declaration
		// - Splits vertices to satisfy 16 bit index limit
		// - Creates grmGeometry from processed vertex data
		List<SplittedGeometry> ConvertSceneGeometry(const graphics::SceneGeometry* sceneGeometry, bool skinned) const;

		// Accounts default material index, returned index can be used for grmShaderGroup / MaterialTuneGroup
		// but not valid for Scene! Use original index for scene.
		u16 GetSceneGeometryMaterialIndex(const graphics::SceneGeometry* sceneGeometry) const;
		const MaterialTune& GetGeometryMaterialTune(const graphics::SceneGeometry* sceneGeometry) const;

		// - Converts single SceneModel to grmModel
		// - Links geometries to shader group
		// - Adds created model to m_SceneToGrmModel
		rage::pgUPtr<rage::grmModel> ConvertSceneModel(const graphics::SceneNode* sceneNode);

		// We have to remap blend indices from scene space to generated skeleton, which can be different
		// Also note that in rage blend indices are stored in float[4], that's not typo
		amUniquePtr<rage::Vector4[]> RemapBlendIndices(const graphics::SceneGeometry* sceneGeometry) const;
		// Apart from skinned skeleton creates non-skinned bones from scene nodes (this is what ZModeler3 does for props / vehicles)
		bool GenerateSkeleton();
		// Links non-skinned models to previously generated bone
		void LinkModelsToSkeleton();
		// We have to insert new 'root' bone if scene has multiple nodes on 'depth 0' (we can have only one root bone)
		// or if the first node has non-zero transform (because root bone can't have transform)
		bool NeedAdditionalRootBone() const;
		// We checkout all collision nodes from skeleton and also allow user to disable unwanted nodes
		// (we have to remember that there maximum can be 128 bones)
		bool CanBecameBone(const graphics::SceneNode* sceneNode) const;
		// Can be called only once skeleton is generated!
		// Returns -1 if node was NULL or not linked to bone
		s32 GetNodeBoneIndex(const graphics::SceneNode* sceneNode) const;

		// Converts scene to rmcDrawable and all child models (nodes) to grmModel's
		// LOD model pipeline is defined by this function
		void SetupLodModels();
		// This is done in the end after posing models from scene because only after that we can have final lod extents
		void CalculateLodExtents() const;

		// Creates grmShader for every scene material (and default one if needed) and sets value from Material::Param
		void CreateMaterials() const;
		bool ResolveAndSetTexture(rage::grcInstanceVar* var, ConstString textureName) const;
		// Sets missing checker texture (similar to half life 2)
		void SetMissingTexture(rage::grcInstanceVar* var) const;

		bool CompileAndSetEmbedDict();

		// Creates collision bound from scene model
		SmallList<rage::phBoundPtr> CreateBoundsFromNode(const graphics::SceneNode* sceneNode) const;
		// Creates composite bound from all collisions in scene, may return NULL if no bounds were created
		void CreateAndSetCompositeBound() const;
		// Checks whether node has postfix .COL
		bool IsCollisionNode(const graphics::SceneNode* sceneNode) const;

		// Transforms model bounding boxes using SceneNode matrix
		// We do this as user-friendly solution to non-zero transform (XForm in 3DS Max)
		void PoseModelBoundsFromScene() const;

		// We allow users to use hex (integer) color instead of solid color texture.
		// To make that possible we create atlas texture with all colors placed on it, and then map UV to it.
		void GeneratePaletteTexture();
		file::WPath GetPaletteTexturePath() const { return m_EmbedDictPath / PALETTE_TEXTURE_NAME; }

		void CreateLights() const;

		bool TryCompileToGame();

		// -- Load utils --

		// Creates folder for embed texture dictionary
		void InitializeEmbedDict();
		// Creates material tunes from scene + default material if needed
		void CreateMaterialTuneGroupFromScene();

	public:
		DrawableAsset(const file::WPath& path);

		bool CompileToGame(gtaDrawable* ppOutGameFormat) override;
		void Refresh() override;

		ConstWString GetXmlName()			const override { return L"Drawable"; }
		ConstWString GetCompileExtension()	const override { return L"ydr"; }
		u32 GetFormatVersion()				const override { return 0; }
		u32 GetResourceVersion()			const override { return 165; }

		void Serialize(XmlHandle& node) const override;
		void Deserialize(const XmlHandle& node) override;

		eAssetType GetType() const override { return AssetType_Drawable; }

		ASSET_IMPLEMENT_ALLOCATE(DrawableAsset);

		// ---------- Asset Related ----------

		TxdAssetPtr GetEmbedDictionary() const { return m_EmbedDictTune; }
		void SetEmbedDictionary(const TxdAssetPtr& dict) { m_EmbedDictTune = dict; }

		graphics::ScenePtr GetScene() const { return m_Scene; }
		void SetScene(const graphics::ScenePtr& scene) { m_Scene = scene; }

		DrawableTune& GetDrawableTune() { return m_DrawableTune; }
		ConstWString GetDrawableModelPath() const { return m_ScenePath; }
	};
}
