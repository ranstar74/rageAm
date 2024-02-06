#pragma once

#include "am/types.h"
#include "am/file/fileutils.h"
#include "am/file/path.h"
#include "am/system/ptr.h"
#include "am/xml/serialize.h"
#include "rage/paging/compiler/compiler.h"

// TODO:
// - Cache system
//		When compiling large problems, we don't want to recompile unchanged asset many times
//		for caching we will need to correctly compute hash of an asset, for TXD
//		we have to account texture files hash-sum + resolved templates

namespace rageam::asset
{
#define ASSET_CONFIG_NAME L"tune.xml"
	// Currently used for easy communication between TXD editor and Hot drawable
#define ASSET_CONFIG_NAME_TEMP L"tune_temp.xml"

	// Must be implemented in all asset classes that are used in AssetFactory
#define ASSET_IMPLEMENT_ALLOCATE(name)									\
	static AssetBase* Allocate(const rageam::file::WPath& path)			\
	{																	\
		return new name(path);											\
	}																	\
	MACRO_END

	using AssetCompileCallback = void(ConstWString message, double percent);

	enum eAssetType
	{
		AssetType_None,
		AssetType_Txd,
		AssetType_Drawable,
	};

	static constexpr ConstWString ASSET_ITD_EXT = L".itd";
	static constexpr ConstWString ASSET_IDR_EXT = L".idr";

	/**
	 * \brief Base class for assets.
	 * \remarks This class is only to be used AssetFactory! Use GameAsset / GameRscAsset for anything else.
	 */
	class AssetBase : public IXml
	{
		file::WPath m_Directory;				// Path to directory where asset-specific files are located
		file::WPath m_WorkspaceDirectory;		// Path to workspace that holds current asset, empty string if none
		HashValue	m_WorkspaceDirectoryHash;
		bool		m_HasSavedConfig;
		u32			m_HashKey;

	public:
		AssetBase(const file::WPath& path);
		AssetBase(const AssetBase& other) = default;
		~AssetBase() override = default;

		// If directory was renamed, workspace has to be updated if directory was moved
		void SetNewPath(ConstWString newPath, bool updateWorkspace = false);

		// Compiles asset into file. If path is null, GetCompilePath() is used.
		virtual bool CompileToFile(ConstWString filePath = nullptr) = 0;
		// Looks up for new and deleted asset source files.
		virtual void Refresh() = 0;
		// Gets current asset format version (not related to game resource version).
		virtual u32 GetFormatVersion() const = 0;
		// Gets full 'technical' asset name, used for root xml tag.
		virtual ConstWString GetXmlName() const = 0;
		// Gets extension of compiled asset - 'ytd', 'ysc', 'yft', etc.
		virtual ConstWString GetCompileExtension() const = 0;

		// Loads asset configuration file from "config.xml", if config doesn't exist - creates default one.
		// Note that this function automatically calls refresh!
		bool LoadConfig(bool temp = false);
		// Saves asset configuration file to "config.xml".
		bool SaveConfig(bool temp = false) const;
		// Useful when some data needs to be set only on config creation
		bool HasSavedConfig() const { return m_HasSavedConfig; }

		// atStringHash of directory path
		u32 GetHashKey() const { return m_HashKey; }
		// Gets full path to asset directory 'x:/assets/adder.itd'
		const file::WPath& GetDirectoryPath() const { return m_Directory; }
		// Path to workspace that holds current asset, empty string if none
		const file::WPath& GetWorkspacePath() const { return m_WorkspaceDirectory; }
		HashValue GetWorkspacePathHash() const { return m_WorkspaceDirectoryHash; }
		// Gets name in format 'adder.itd'
		ConstWString GetAssetName() const { return file::GetFileName(m_Directory.GetCStr()); }
		// Gets full path to asset config 'x:/assets/adder.itd/config.xml', config name is defined by ASSET_CONFIG_NAME
		file::WPath GetConfigPath(bool temp = false) const { return m_Directory / (temp ? ASSET_CONFIG_NAME_TEMP : ASSET_CONFIG_NAME); }
		// Gets default path where asset is compiled, 'x:/assets/adder.itd' will compile into 'x:/assets/adder.ytd' binary
		file::WPath GetCompilePath() const
		{
			// resources/example.itd -> resources/example
			file::WPath path = m_Directory.GetFilePathWithoutExtension();
			// resources/example.ytd
			path += L".";
			path += GetCompileExtension();
			return path;
		}

		
		virtual HashValue ComputeHashKey() { return 0; }

		virtual eAssetType GetType() const = 0;

		// Invoked by asset during compilation process, sort of:
		// - adder_badges,	11%
		// - adder_lights,	23%
		// Note: This is not thread-safe function! Most likely going to be invoked from BackgroundWorker threads.
		std::function<AssetCompileCallback> CompileCallback;

	protected:
		void ReportProgress(const wchar_t* message, double progress) const
		{
			if (!CompileCallback)
				return;
			CompileCallback(message, progress);
		}
	};
	using AssetPtr = amPtr<AssetBase>;

	/**
	 * \brief File that is being part of resource.
	 * For texture dictionary such file will be an image file.
	 */
	class AssetSource : IXml
	{
		AssetBase*  m_Parent;
		file::WPath m_FilePath; // Full file path including extension
		u32			m_HashKey;

	public:
		AssetSource(AssetBase* parent, ConstWString filePath);
		AssetSource(const AssetSource&) = default;
		~AssetSource() override = default;

		// Gets base asset resource this source file belongs to
		AssetBase* GetParent() const { return m_Parent; }

		ConstWString GetName() const { return file::GetFileName(m_FilePath.GetCStr()); }
		void         SetFilePath(ConstWString path);
		ConstWString GetFilePath() const { return m_FilePath; }
		u32          GetHashKey() const { return m_HashKey; }

		static u32 ComputeHashKey(ConstWString path) { return Hash(path); }

		bool operator==(const AssetSource& other) const { return m_HashKey == other.m_HashKey; }
		AssetSource& operator=(const AssetSource&) = default;
	};

	/**
	 * \brief Base class for game assets.
	 */
	template<typename TGameFormat>
	class GameAsset : public AssetBase
	{
	public:
		GameAsset(const file::WPath& path) : AssetBase(path) {}
		GameAsset(const GameAsset& other) = default;

		// Compiles asset into game-compatible format for raw streaming.
		virtual bool CompileToGame(TGameFormat* object) = 0;
		// Creates config (or updates existing) from given game resource, basically decompiling.
		// For drawable:
		// In case if drawable was previously compiled,
		// the map will be used to properly match existing tune with compiled resource
		virtual void ParseFromGame(TGameFormat* object) = 0;
	};

	/**
	 * \brief Base class for resource (RSC7 / Paged) assets.
	 */
	template<typename TGameFormat>
	class GameRscAsset : public GameAsset<TGameFormat>
	{
	public:
		GameRscAsset(const file::WPath& path) : GameAsset<TGameFormat>(path) {}
		GameRscAsset(const GameRscAsset& other) = default;

		bool CompileToFile(ConstWString filePath = nullptr) override
		{
			AM_TRACEF(L"Compiling game asset %ls", this->GetDirectoryPath());

			TGameFormat gameFormat;
			if (!AM_VERIFY(this->CompileToGame(&gameFormat), "GameRscAsset::CompileToFile() -> Failed to compile game format..."))
				return false;

			file::WPath compilePath;
			if (filePath)
				compilePath = filePath;
			else
				compilePath = this->GetCompilePath();

			this->ReportProgress(L"- Compiling resource", 0);

			rage::pgRscCompiler compiler;
			compiler.CompileCallback = [this](ConstWString message, double progress)
				{
					this->ReportProgress(message, progress);
				};

			return compiler.Compile(&gameFormat, GetResourceVersion(), compilePath);
		}

		// Hot-reload part of asset, if possible.
		virtual void ApplyChange(TGameFormat* resource, int changeId) {}

		// RORC (Rockstar Offline Resource Compiler) version from resource header.
		virtual u32 GetResourceVersion() const = 0;
	};
}
