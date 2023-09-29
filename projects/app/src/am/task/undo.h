//
// File: undo.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include <functional>

#include "am/system/ptr.h"
#include "rage/atl/array.h"
#include "rage/atl/fixedarray.h"

namespace rageam
{
	/**
	 * \brief Undoable action.
	 */
	class IUndoable
	{
		friend class UndoStack;

	protected:
		virtual void Do() = 0;
		virtual void Undo() = 0;
	public:
		virtual ~IUndoable() = default;
	};

	/**
	 * \brief Undoable action that supports lambda functions.
	 */
	class UndoableFn : public IUndoable
	{
		friend class UndoStack;

		using TFn = std::function<void()>;

		TFn m_Do;
		TFn m_Undo;

		void Do() override { m_Do(); }
		void Undo() override { m_Undo(); }
	public:
		UndoableFn(TFn doFun, TFn undoFun) : m_Do(std::move(doFun)), m_Undo(std::move(undoFun))
		{

		}
	};

	template<typename T>
	class UndoableState : public IUndoable
	{
		using TCallback = std::function<void()>;

		T& m_State;

		T m_OldState;
		T m_NewState;

		TCallback m_Callback;

		void Do() override
		{
			m_State = m_NewState;

			if (m_Callback) m_Callback();
		}

		void Undo() override
		{
			m_State = m_OldState;

			if (m_Callback) m_Callback();
		}
	public:
		UndoableState(T& oldState, const T& newState)
			: m_State(oldState), m_OldState(std::move(oldState)), m_NewState(std::move(newState))
		{

		}

		UndoableState(T& oldState, const T& newState, TCallback callback)
			: UndoableState(oldState, newState)
		{
			m_Callback = std::move(callback);
		}
	};

	enum eUndoStackOptions
	{
		UNDO_CONTEXT_NONE = 0,
		UNDO_CONTEXT_HARD_REDO = 1 << 1,	// New action will erase redo history
	};

	/**
	 * \brief Undo action system for UI.
	 */
	class UndoStack
	{
		static constexpr u32 MAX_HISTORY_SIZE = 128;
		static constexpr u32 UNDO_STACK_SIZE = 32;

		rage::atFixedArray<amUniquePtr<IUndoable>, MAX_HISTORY_SIZE> m_History;

		// 'Last' index of completed action in history.
		// When we undo/redo action, this index is decremented/incremented,
		// so it allows us to 'walk' on history timeline.
		u16 m_HistoryPoint = 0;

		// History point that is considered as 'Saved' one.
		u16 m_SavePoint = 0;

		FlagSet<eUndoStackOptions> m_Options;

		static rage::atFixedArray<UndoStack*, UNDO_STACK_SIZE> sm_ContextStack;
	public:
		UndoStack(FlagSet<eUndoStackOptions> options = UNDO_CONTEXT_NONE);

		bool Any() const { return m_History.Any(); }
		bool CanUndo() const { return m_HistoryPoint > 0; }
		bool CanRedo() const { return m_HistoryPoint < m_History.GetSize(); }
		void Clear();

		void Redo();
		void Undo();

		void Add(IUndoable* action);
		void AddFn(UndoableFn::TFn doFn, UndoableFn::TFn undoFn);

		// Gets whether current undo position is located at save point, see MarkSavePoint();
		bool HasChanges() const { return m_HistoryPoint != m_SavePoint; }
		// Sets current undo position as 'Saved', so HasChanges() will return true.
		void SetSavePoint() { m_SavePoint = m_HistoryPoint; }

		static UndoStack* GetCurrent()
		{
			if (!sm_ContextStack.Any())
				return nullptr; // We allow no active undo stack
			return sm_ContextStack.Last();
		}
		static void Push(UndoStack& undo) { sm_ContextStack.Add(&undo); }
		static void Pop() { sm_ContextStack.RemoveLast(); }
	};
}
