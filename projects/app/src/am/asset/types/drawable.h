//
// File: drawable.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "txd.h"
#include "scenetunegroup.h"
#include "am/asset/gameasset.h"
#include "am/asset/workspace.h"
#include "am/graphics/scene.h"
#include "game/drawable.h"
#include "am/types.h"

#include <any>

namespace rageam::asset
{
	// Version History:
	// 0: Initial

	// TODO:
	// - Auto color-texture map generation for models that don't have texture
	//		but only single color, to make life easier
	// - We don't really have TXD parenting, it might be confusing for some users,
	//		we might want to generate txd relationship xml
	// - We might want to compile only used textures, or at least warn user about unused ones

	static constexpr u16          MAX_BONES = 128;
	static constexpr int          MAX_LOD = 4;
	static constexpr ConstString  DEFAULT_SHADER = "default"; // default.fxc
	static constexpr ConstString  DEFAULT_MATERIAL_NAME = "$am_default_material";
	static constexpr ConstWString PALETTE_TEXTURE_NAME = L"autogen_palette";
	static constexpr ConstWString EMBED_DICT_NAME = L"textures";
	static constexpr ConstString  COL_MODEL_EXT = ".COL"; // Postfix of collision models in scene

	/**
	 * \brief Helper to hold drawable tex dict dependency.
	 */
	struct DrawableTxd
	{
		TxdAssetPtr					  Asset;
		rage::grcTextureDictionaryPtr Dict;	    // May be null if failed to compile
		bool						  IsEmbed;

		bool TryCompile();
	};
	struct DrawableTxdHashFn
	{
		u32 operator()(const DrawableTxd& txd) const { return txd.Asset->GetHashKey(); }
	};
	using DrawableTxdSet = HashSet<DrawableTxd, DrawableTxdHashFn>;

	struct MaterialTune : SceneTune
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

		string		Effect;
		u8			DrawBucket = 0;
		List<Param>	Params;
		// Flag for UI to mark orphan material as deleted and ignore it on writing config,
		// we do this instead of deleting it because number of materials in tune
		// must match shader group for direct mapping
		bool		NoLongerNeeded = false;
		bool		IsDefault;

		MaterialTune() = default;
		MaterialTune(ConstString name, bool isDefault, ConstString shaderName = DEFAULT_SHADER)
		{
			Name = name;
			Effect = shaderName;
			IsDefault = isDefault;
		}
		MaterialTune(const MaterialTune& other) = default;

		void Deserialize(const XmlHandle& node) override;
		void Serialize(XmlHandle& node) const override;

		// Finds effect from ShaderHash
		rage::grcEffect* LookupEffect() const;
		// Attempts to look up effect and uses DEFAULT_SHADER if not found
		rage::grcEffect* LookupEffectOrUseDefault() const;

		// Resets current parameters and defaults them from specified shader
		void InitFromEffect();
		// Parses parameters from given shader instance
		void InitFromShader(const rage::grcInstanceData* shader);
		// Attempts to match texture name from material to 'DiffuseSampler' or 'SpecSampler' effect params
		void SetTextureNamesFromSceneMaterial(const graphics::SceneMaterial* sceneMaterial);

		SceneTunePtr Clone() const override { return std::make_shared<MaterialTune>(*this); }
	};
	
	struct MaterialTuneGroup : SceneTuneGroup<MaterialTune>
	{
		bool         ExistsInScene(graphics::Scene* scene, const SceneTunePtr& tune) const override;
		int          IndexOf(const graphics::Scene* scene, ConstString itemName) const override;
		SceneTunePtr CreateTune() const override;
		SceneTunePtr CreateDefaultTune(graphics::Scene* scene, u16 itemIndex) const override;
		ConstString  GetItemName(graphics::Scene* scene, u16 itemIndex) const override;
		u16          GetSceneItemCount(graphics::Scene* scene) const override;
		ConstString  GetName() const override { return "Materials"; }

		MaterialTuneGroup() = default;
		MaterialTuneGroup(const MaterialTuneGroup& other) = default;
	};

	struct ModelTune : SceneTune
	{
		bool	UseAsBound = false;			// Uses least detailed lod as bound
		bool	CreateBone = false;			// Force create bone from the node
		//bool	EnableShadows = true;
		//bool	ShowInReflections = true;

		void Serialize(XmlHandle& node) const override;
		void Deserialize(const XmlHandle& node) override;

		SceneTunePtr Clone() const override { return std::make_shared<ModelTune>(*this); }
	};

	struct ModelTuneGroup : SceneTuneGroup<ModelTune>
	{
		bool         ExistsInScene(graphics::Scene* scene, const SceneTunePtr& tune) const override;
		int          IndexOf(const graphics::Scene* scene, ConstString itemName) const override;
		SceneTunePtr CreateTune() const override;
		SceneTunePtr CreateDefaultTune(graphics::Scene* scene, u16 itemIndex) const override;
		ConstString  GetItemName(graphics::Scene* scene, u16 itemIndex) const override;
		u16          GetSceneItemCount(graphics::Scene* scene) const override;
		ConstString  GetName() const override { return "Models"; }
	};

	struct LightTune : SceneTune
	{
		enum LightType { Point = 1, Spot = 2, Capsule = 4, };

		// -- Common
		LightType			Type = Point;
		Vec3S				Position;
		Vec3S				Direction;
		Vec3S				Tangent;
		graphics::ColorU32	ColorRGB;
		u8					Flashiness;
		u32					Flags;
		u32					TimeFlags;
		float				Intensity;
		float				Falloff;
		float				FalloffExponent;
		Vec3S				CullingPlaneNormal;
		float				CullingPlaneOffset;
		u8					ShadowBlur;
		float				ShadowNearClip;
		float				CoronaSize;
		float				CoronaIntensity;
		float				CoronaZBias;
		u8					LightHash;				// TODO: Not implemented
		string				ProjectedTexture;		// TODO: Not implemented
		u8					LightFadeDistance;
		u8					ShadowFadeDistance;
		u8					SpecularFadeDistance;
		u8					VolumetricFadeDistance;

		// -- Spot
		float				VolumeIntensity;
		float				VolumeSizeScale;
		float				VolumeOuterIntensity;
		float				VolumeOuterExponent;
		graphics::ColorU32	VolumeOuterColorRGB;
		float				ConeInnerAngle;
		float				ConeOuterAngle;

		// -- Capsule
		float				CapsuleLength;

		void Serialize(XmlHandle& node) const override;
		void Deserialize(const XmlHandle& node) override;

		void InitFromSceneLight(const graphics::SceneNode* lightNode);
		void FromLightAttr(const CLightAttr* attr);

		SceneTunePtr Clone() const override { return std::make_shared<LightTune>(*this); }
	};

	struct LightTuneGroup : SceneTuneGroup<LightTune>
	{
		bool         ExistsInScene(graphics::Scene* scene, const SceneTunePtr& tune) const override;
		int			 IndexOf(const graphics::Scene* scene, ConstString itemName) const override;
		SceneTunePtr CreateTune() const override;
		SceneTunePtr CreateDefaultTune(graphics::Scene* scene, u16 itemIndex) const override;
		ConstString  GetItemName(graphics::Scene* scene, u16 itemIndex) const override;
		u16          GetSceneItemCount(graphics::Scene* scene) const override;
		ConstString  GetName() const override { return "Lights"; }

		LightTuneGroup() = default;
		LightTuneGroup(const LightTuneGroup& other) = default;
	};

	struct LodGroupTune : IXml
	{
		string			FileName;
		ModelTuneGroup	Models;
		float			LodThreshold[MAX_LOD] = { 30, 60, 90, 120 };
		bool			AutoGenerateLods = false; // TODO: We need some 3D mesh processing library

		LodGroupTune() = default;
		LodGroupTune(const LodGroupTune& other) = default;

		void Serialize(XmlHandle& node) const override;
		void Deserialize(const XmlHandle& node) override;
	};

	struct DrawableTune : IXml
	{
		file::WPath       SceneFileName;
		MaterialTuneGroup Materials;
		LightTuneGroup    Lights;
		LodGroupTune      Lods;

		DrawableTune() = default;
		DrawableTune(const DrawableTune& other) = default;

		void Serialize(XmlHandle& node) const override;
		void Deserialize(const XmlHandle& node) override;
	};

	/**
	 * \brief Maps 3D scene data to compiled gta drawable.
	 */
	struct DrawableAssetMap
	{
		// SceneNode -> ### can be NULL
		// grmModel/CLightAttr ALWAYS map to a valid scene index
		// BoneToSceneNode MAY map to NULL index for autogenerated root bone
		// Shader WILL map to NULL index for implicit default material

		struct ModelHandle { u16 Lod, Index; };

		SmallList<ModelHandle>	SceneNodeToModel;		// graphics::SceneNode		-> rage::grmModel
		SmallList<u16>			SceneNodeToBound;		// graphics::SceneNode		-> rage::phBound
		SmallList<u16>			SceneNodeToBone;		// graphics::SceneNode		-> rage::crBoneData
		SmallList<u16>			SceneMaterialToShader;	// graphics::SceneMaterial	-> rage::grmShader
		SmallList<u16>			SceneNodeToLightAttr;	// graphics::SceneLight		-> CLightAttr

		SmallList<u16>			ModelToSceneNode;		// rage::grmModel			-> graphics::SceneNode
		SmallList<u16>			BoundToSceneNode;		// rage::phBound			-> graphics::SceneNode
		SmallList<u16>			BoneToSceneNode;		// rage::crBoneData			-> graphics::SceneNode
		SmallList<u16>			ShaderToSceneMaterial;	// rage::grmShader			-> graphics::SceneMaterial
		SmallList<u16>			LightAttrToSceneNode;	// CLightAttr				-> graphics::SceneLight

		rage::grmModel*   GetModelFromScene(rage::rmcDrawable* drawable, u16 sceneNodeIndex) const;
		rage::crBoneData* GetBoneFromScene(const rage::rmcDrawable* drawable, u16 sceneNodeIndex) const;
		rage::phBound*    GetBoundFromScene(const gtaDrawable* drawable, u16 sceneNodeIndex) const;
		CLightAttr*       GetLightFromScene(gtaDrawable* drawable, u16 sceneNodeIndex) const;
	};

	class DrawableAsset : public GameRscAsset<gtaDrawable>
	{
		// Path's used only if loaded from xml

		file::WPath			m_EmbedDictPath;	 // By default (e.g. for 'bender.idr') will be 'bender.idr/bender.itd'
		TxdAssetPtr			m_EmbedDictTune;
		graphics::ScenePtr	m_Scene;
		u64					m_SceneFileTime = 0; // To reload scene on compiling if it was modified
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

		gtaDrawable*				m_Drawable = nullptr;
		// NOTE: May be null if drawable has no embed textures
		// No smart pointer here because pointer ownership will be passed to drawable
		rage::grcTextureDictionary* m_EmbedDict = nullptr;
		// We cache reflected vertex declaration & fvf from effect here (key is effect name without .fxc extension)
		HashSet<EffectInfo>			m_EffectCache;
		// Key is SceneNode index
		List<rage::grmModel*>		m_NodeToModel;
		List<rage::crBoneData*>		m_NodeToBone;

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
		void CreateMaterials();
		bool ResolveAndSetTexture(rage::grcInstanceVar* var, ConstString textureName);
		// Sets missing checker texture (similar to half life 2)
		void SetMissingTexture(rage::grcInstanceVar* var, ConstString textureName) const;
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

		// Creates light attributes from scene lights
		void CreateLights();

		bool ValidateScene() const;

		bool TryCompileToGame();

		// -- Load utils --

		// Creates folder for embed texture dictionary
		void RefreshEmbedDict();
		// Creates material tunes from scene + default material if needed
		//void CreateMaterialTuneGroupFromScene();
		// Loads shared texture dictionaries from workspace
		void RefreshTXDWorkspace();
		// Attempts to find first 3D file in asset directory
		bool TryToFindFirstSceneFile(file::WPath& outPath) const;
		// Verifies that currently specified model file in scene exists or scans for first one in asset directory
		bool RefreshSceneFile();
		// Rebuilds tune for current scene model
		void RefreshTuneFromScene();

	public:
		DrawableAsset(const file::WPath& path);
		DrawableAsset(const DrawableAsset& other);

		bool CompileToGame(gtaDrawable* ppOutGameFormat) override;
		void ParseFromGame(gtaDrawable* object) override;
		void Refresh() override;

		void RefreshTunesFromScene();

		ConstWString GetXmlName()			const override { return L"Drawable"; }
		ConstWString GetCompileExtension()	const override { return L"ydr"; }
		u32 GetFormatVersion()				const override { return 0; }
		u32 GetResourceVersion()			const override { return 165; }

		void Serialize(XmlHandle& node) const override;
		void Deserialize(const XmlHandle& node) override;

		eAssetType GetType() const override { return AssetType_Drawable; }

		ASSET_IMPLEMENT_ALLOCATE(DrawableAsset);

		// ---------- Asset Related ----------

		const file::WPath& GetEmbedDictionaryPath() const { return m_EmbedDictPath; }
		TxdAssetPtr        GetEmbedDictionary() const { return m_EmbedDictTune; }
		void               SetEmbedDictionary(const TxdAssetPtr& dict) { m_EmbedDictTune = dict; }

		graphics::ScenePtr GetScene() const { return m_Scene; }
		void               SetScene(const graphics::ScenePtr& scene) { m_Scene = scene; }

		DrawableTune& GetDrawableTune() { return m_DrawableTune; }

		// File to the scene 3D model file, might be empty
		file::WPath GetScenePath() const { return GetDirectoryPath() / m_DrawableTune.SceneFileName; }
		// NOTE: There's no checks if path is valid or not, used only by hot reload currently
		void		SetScenePath(const file::WPath& path) { m_DrawableTune.SceneFileName = path.GetFileName(); }

		// Only to be used if asset was compiled successfully!
		amUPtr<DrawableAssetMap> CompiledDrawableMap;
		// Workspace with shared texture dictionaries loaded, may be NULL
		WorkspacePtr			 WorkspaceTXD;
		// TXDs with textures that are used by drawable materials (NOT including embed TXD)
		// NOTE: Destroying asset will destroy those dictionaries and all the textures!
		DrawableTxdSet			 SharedTXDs;

		// Uses missing textures instead of resolving them from texture dictionaries,
		// all missing textures are added in embed dictionary
		static inline thread_local bool tl_SkipTextures = false;
	};
	using DrawableAssetPtr = amPtr<DrawableAsset>;
}
