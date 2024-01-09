//
// File: entryselection.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "entry.h"
#include "rage/atl/set.h"

namespace rageam::ui
{
	/**
	 * \brief Entry selection set.
	 */
	class EntrySelection
	{
		using Entry = ExplorerEntryPtr;

		rage::atSet<Entry, ExplorerEntryPtrHashFn> m_Selections;
	public:
		EntrySelection() = default;
		EntrySelection(const std::initializer_list<Entry>& list) : m_Selections(list) {}
		EntrySelection(const EntrySelection&) = default;
		EntrySelection(EntrySelection&&) = default;

		void AllocateForDirectory(const Entry& entry)
		{
			m_Selections.Resize(entry->GetChildCount());
		}

		void ClearSelection()
		{
			m_Selections.Clear();
		}

		void SetSelected(const Entry& entry, bool toggle)
		{
			if (toggle)
			{
				m_Selections.Insert(entry);
			}
			else
			{
				auto iterator = m_Selections.Find(entry);
				if (iterator != m_Selections.end())
					m_Selections.RemoveAtIterator(iterator);
			}
		}

		bool IsSelected(const Entry& entry) const
		{
			return m_Selections.Contains(entry);
		}

		void ToggleSelection(const Entry& entry)
		{
			auto iterator = m_Selections.Find(entry);
			if (iterator == m_Selections.end())
				m_Selections.Insert(entry);
			else
				m_Selections.RemoveAtIterator(iterator);
		}

		bool Any() const { return m_Selections.Any(); }
		u16 GetCount() const { return m_Selections.GetNumUsedSlots(); }

		EntrySelection& operator=(const EntrySelection& other) = default;
		EntrySelection& operator=(EntrySelection&&) = default;

		bool operator==(const EntrySelection& other) const = default;

		auto begin() const { return m_Selections.begin(); }
		auto end() const { return m_Selections.end(); }
	};
}
