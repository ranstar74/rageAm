//
// File: entry.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "entry.h"
#include "imgui.h"
#include "am/time/datetime.h"
#include "am/asset/gameasset.h"
#include "am/ui/context.h"
#include "rage/atl/array.h"
#include "rage/file/device.h"

namespace rageam::ui
{
	// TODO: Remove deprecated ID's and replace them completely with hash keys

	static constexpr eIconSize ExplorerEntrySmallIcon = IconSize_16;
	static constexpr eIconSize ExplorerEntryLargeIcon = IconSize_256;

	class Image;
	class IExplorerEntry;
	using ExplorerEntryPtr = amPtr<IExplorerEntry>;

	enum ExplorerEntryColumnID
	{
		ExplorerEntryColumnID_Name = 0,
		ExplorerEntryColumnID_DateModified = 1,
		ExplorerEntryColumnID_TypeName = 2,
		ExplorerEntryColumnID_Size = 3,
	};

	enum ExplorerEntryType
	{
		ExplorerEntryType_Fi,
		ExplorerEntryType_User,
	};

	enum ExplorerEntryFlags_
	{
		ExplorerEntryFlags_None = 0,
		ExplorerEntryFlags_CantBeRemoved = 1 << 0, // Will disable 'Delete' option in UI
		ExplorerEntryFlags_NoRename = 1 << 1,
	};
	using ExplorerEntryFlags = int;

	static constexpr ConstString ExplorerEntryDragType = "ExplorerEntryDragType";
	struct ExplorerEntryDragData
	{
		rage::atArray<ExplorerEntryPtr> Entries;
		bool Cut;
	};

	// Allows this entry to be dragged, outDragData will be set to non-null pointer when data fill is required
	// Returns true if entry is being dragged
	bool ExplorerEntryBeginDragSource(const ExplorerEntryPtr& entry, ExplorerEntryDragData** outDragData);
	// Must be called if ExplorerEntryEndDragDropSource returned True.
	void ExplorerEntryEndDragSource();
	// Allows an entry to be drag&dropped on given entry
	void ExplorerEntryDragTargetBehaviour(const ExplorerEntryPtr& entry);

	class IExplorerEntry
	{
	public:
		virtual ~IExplorerEntry() = default;

		virtual IExplorerEntry* GetParent() const = 0;
		virtual void SetParent(IExplorerEntry* parent) = 0;

		virtual u32 GetHashKey() const = 0;					// Used currently only for selection set
		virtual u16 GetID() const = 0;						// Unique (in directory space) index, not affected by sorting
		virtual void SetID(u16 id) = 0;

		virtual ExplorerEntryFlags GetFlags() = 0;
		virtual void SetFlags(ExplorerEntryFlags flags) = 0;

		virtual void Refresh() = 0;							// Rescans all available information
		virtual void RefreshDisplayInfo() = 0;				// May update formatted time or any other temporal variable

		virtual void PrepareToBeDisplayed() = 0;			// We load resource-heavy information (icons, strings) only if entry is visible

		virtual bool Rename(ConstString newName) = 0;

		virtual ConstString GetCustomName() const = 0;
		virtual void SetCustomName(ConstString name) = 0;

		virtual ConstString GetType() const = 0;			// File extension
		virtual ConstString GetTypeName() const = 0;		// Display name for extension (Type column in Windows explorer)
		virtual ConstString GetPath() const = 0;			// Absolute path including name and extension
		virtual ConstString GetName() const = 0;			// Name without extension
		virtual ConstString GetFullName() const = 0;		// Name including extension

		virtual DateTime GetTime() const = 0;				// Last modify time
		virtual ConstString GetTimeDisplay() const = 0;		// Last modify time as formatted string

		virtual u32 GetSize() const = 0;

		virtual bool IsDirectory() const = 0;
		virtual bool HasChildDirectories() const = 0;		// Used by tree view to quickly detect leaf nodes
		virtual void LoadChildren() = 0;
		virtual void UnloadChildren() = 0;
		virtual u16 GetChildCount() const = 0;
		virtual u16 GetIndexFromID(u16 id) const = 0;
		virtual u16 GetIndexFromHash(u32 hash) const = 0;
		virtual u16 TransformToSorted(u16 index) = 0;
		virtual u16 TransformFromSorted(u16 index) = 0;
		virtual ExplorerEntryPtr& GetChildFromIndex(u16 index) = 0;
		virtual ExplorerEntryPtr& GetChildFromID(u16 id) = 0;
		virtual ExplorerEntryPtr& GetSortedChildFromIndex(u16 index) = 0;

		virtual void Sort(ImGuiTableSortSpecs* specs) = 0;

		virtual void SetIconOverride(ConstString name) = 0;
		virtual const Image& GetIcon() const = 0;			// 16x16
		virtual const Image& GetLargeIcon() const = 0;		// At least 256x256

		virtual bool IsAsset() const = 0;
		virtual asset::AssetPtr GetAsset() = 0;

		virtual ExplorerEntryType GetEntryType() = 0;

		virtual void SetUserData(u32 key, u32 value) = 0;
		virtual u32 GetUserData(u32 key) = 0;

		virtual ExplorerEntryPtr* begin() = 0;				// Pointer to children array
		virtual ExplorerEntryPtr* end() = 0;

		// Gets whether entry parent is ExplorerEntryUser
		bool IsUserParent() const { return GetParent() && GetParent()->GetEntryType() == ExplorerEntryType_User; }
	};

	struct ExplorerEntryPtrHashFn
	{
		u32 operator()(const ExplorerEntryPtr& entry) const
		{
			return entry->GetHashKey();
		}
	};

	/**
	 * \brief Entry that implements children array and other common things.
	 */
	class ExplorerEntryBase : public IExplorerEntry
	{
	protected:
		IExplorerEntry* m_Parent = nullptr;

		// Index of entry in parent array (if there's any), used to get actual index of item after sorting
		u16	m_ID = 0;

		rage::atArray<ExplorerEntryPtr> m_Children;
		// Instead of sorting children array we sort child indices
		// This maps sorted index to index in m_Children
		rage::atArray<u16> m_SortedIndexToEntry;

		// u32 -> u32 map, can be used to store indices
		rage::atSet<u32> m_UserData;

		ExplorerEntryFlags m_Flags = ExplorerEntryFlags_None;

	public:
		IExplorerEntry* GetParent() const override { return m_Parent; }
		void SetParent(IExplorerEntry* parent) override { m_Parent = parent; }

		u16 GetID() const override { return m_ID; }
		void SetID(u16 id) override { m_ID = id; }

		ExplorerEntryFlags GetFlags() override { return m_Flags; }
		void SetFlags(ExplorerEntryFlags flags) override { m_Flags = flags; }

		u16 GetChildCount() const override { return m_Children.GetSize(); }
		u16 GetIndexFromID(u16 id) const override;
		u16 GetIndexFromHash(u32 hash) const override;
		u16 TransformToSorted(u16 index) override;
		u16 TransformFromSorted(u16 index) override;
		ExplorerEntryPtr& GetChildFromIndex(u16 index) override;
		ExplorerEntryPtr& GetChildFromID(u16 id) override;
		ExplorerEntryPtr& GetSortedChildFromIndex(u16 index) override;

		void Sort(ImGuiTableSortSpecs* specs) override;

		void SetUserData(u32 key, u32 value) override;
		u32 GetUserData(u32 key) override;

		ExplorerEntryPtr* begin() override { return m_Children.begin(); }
		ExplorerEntryPtr* end() override { return m_Children.end(); }
	};

	/**
	 * \brief Entry powered by rage file device.
	 */
	class ExplorerEntryFi : public ExplorerEntryBase
	{
		static constexpr u32 USER_MAX_NAME = 64;
		static constexpr u32 TYPE_MAX_NAME = 64;
		static constexpr u32 MAX_TIME = 32;

		rage::fiDevicePtr	m_Device = nullptr;				// We use rage device because in future there will be pack file support

		char				m_UserName[USER_MAX_NAME]{};	// User-defined name
		char				m_TypeName[TYPE_MAX_NAME];		// Display name of file extension
		file::U8Path		m_Path;							// Full path to this file or directory, including name
		file::Path			m_Name;							// Name without extension, not UTF8/16 because they're not compatible with fiDevice
		u32					m_HashKey;						// Joaat of file path, we use that for selection set

		ConstString			m_FullName;						// Pointer to file name in m_Path
		ConstString			m_Type;							// Pointer to file extension in m_Path

		char				m_TimeModifiedFormatted[MAX_TIME];
		DateTime			m_TimeModified;

		u32					m_Size;
		u32					m_Attributes;

		bool				m_IsDirectory;
		bool				m_ChildrenLoaded = false;
		bool				m_HasSubFolders = false;

		// To load/reload info when entry becomes visible on screen
		bool				m_IconDirty = true;
		bool				m_DisplayInfoDirty = true;
		bool				m_HasSubFoldersDirty = true;

		bool				m_IsAsset;
		asset::AssetPtr		m_Asset;						// We load & store asset here to easily open it from explorer folder view

		ConstString m_IconOverride = nullptr;
		Image m_DynamicIcon;								// Dynamic file icon for images
		Image* m_StaticIcon = nullptr;						// Static icon from 'data/icons'
		Image* m_StaticLargeIcon = nullptr;

		void ScanSubFolders();
		void SetPath(const file::U8Path& path);
		void UpdateIcon();
	public:
		ExplorerEntryFi(const file::U8Path& path, ExplorerEntryFlags flags = 0);
		ExplorerEntryFi(ExplorerEntryFi& other) = delete;
		~ExplorerEntryFi() override = default;

		u32 GetHashKey() const override { return m_HashKey; }

		void Refresh() override;
		void RefreshDisplayInfo() override;	// Updates formatted modify time (e.g. now or 5 minutes ago)

		void PrepareToBeDisplayed() override;

		ConstString GetCustomName() const override { return m_UserName; }
		void SetCustomName(ConstString name) override { String::Copy(m_UserName, USER_MAX_NAME, name); }

		ConstString GetType() const override { return m_Type; }				// File extension
		ConstString GetTypeName() const override { return m_TypeName; }		// Display name for extension (Type column in Windows explorer)
		ConstString GetPath() const override { return m_Path; }				// Absolute path including name and extension
		ConstString GetName() const override { return m_Name; }				// Name without extension
		ConstString GetFullName() const override { return m_FullName; }		// Name including extension

		DateTime GetTime() const override { return m_TimeModified; }
		ConstString GetTimeDisplay() const override { return m_TimeModifiedFormatted; }

		u32 GetSize() const override { return m_Size; }

		bool IsDirectory() const override { return m_IsDirectory; }
		bool HasChildDirectories() const override { return m_HasSubFolders; }
		void LoadChildren() override;
		void UnloadChildren() override;

		bool Rename(ConstString newName) override;

		void SetIconOverride(ConstString name) override;
		const Image& GetIcon() const override { return m_StaticIcon != nullptr ? *m_StaticIcon : m_DynamicIcon; }
		const Image& GetLargeIcon() const override { return m_StaticLargeIcon != nullptr ? *m_StaticLargeIcon : m_DynamicIcon; }

		bool IsAsset() const override { return m_IsAsset; }
		asset::AssetPtr GetAsset() override;

		ExplorerEntryType GetEntryType() override { return ExplorerEntryType_Fi; }
	};

	/**
	 * \brief User-defined entry, works as virtual directory and allows to manage custom user collections.
	 */
	class ExplorerEntryUser : public ExplorerEntryBase
	{
		static constexpr u32 MAX_USER_NAME = 32;

		char	m_Name[MAX_USER_NAME];	// Custom display name that user can set, used for disk drives
		u32		m_HashKey;				// Name joaat
		bool	m_HasSubDirs = false;

		Image* m_Icon;
		Image* m_LargeIcon;

		void ScanSubDirs();
	public:
		ExplorerEntryUser(ConstString name)
		{
			ExplorerEntryUser::Rename(name);
			SetIcon("folder");
		}

		u32 GetHashKey() const override { return m_HashKey; }

		void Refresh() override {}
		void RefreshDisplayInfo() override {}

		void PrepareToBeDisplayed() override {}

		bool Rename(ConstString newName) override;

		ConstString GetCustomName() const override { return m_Name; }
		void SetCustomName(ConstString name) override { Rename(name); }

		ConstString GetType() const override { return ""; }
		ConstString GetTypeName() const override { return ""; }
		ConstString GetPath() const override { return m_Name; }
		ConstString GetName() const override { return m_Name; }
		ConstString GetFullName() const override { return m_Name; }

		DateTime GetTime() const override { return 0; }
		ConstString GetTimeDisplay() const override { return ""; }

		u32 GetSize() const override { return 0; }

		bool IsDirectory() const override { return true; }
		bool HasChildDirectories() const override { return m_HasSubDirs; }
		void LoadChildren() override {}
		void UnloadChildren() override {}

		bool IsAsset() const override { return false; }
		asset::AssetPtr GetAsset() override { AM_UNREACHABLE("ExplorerEntryUser::GetAsset() -> Not supported."); }

		void SetIconOverride(ConstString name) override { SetIcon(name); }
		Image& GetIcon() const override { return *m_Icon; }
		Image& GetLargeIcon() const override { return *m_LargeIcon; }

		ExplorerEntryType GetEntryType() override { return ExplorerEntryType_User; }

		// User-Specific

		ExplorerEntryPtr& InsertChildren(u16 index, const ExplorerEntryPtr& entry);
		ExplorerEntryPtr& AddChildren(const ExplorerEntryPtr& entry);
		void RemoveChildren(const ExplorerEntryPtr& entry);
		void RemoveChildrenAtIndex(u16 index);
		void SetIcon(ConstString name)
		{
			m_Icon = Gui->Icons.GetIcon(name, ExplorerEntrySmallIcon);
			m_LargeIcon = Gui->Icons.GetIcon(name, ExplorerEntryLargeIcon);
		}
	};
	using ExplorerEntryUserPtr = amPtr<ExplorerEntryUser>;
}
