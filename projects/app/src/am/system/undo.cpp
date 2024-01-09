#include "undo.h"

rage::atFixedArray<
	rageam::UndoStack*,
	rageam::UndoStack::UNDO_STACK_SIZE> rageam::UndoStack::sm_ContextStack;

rageam::UndoStack::UndoStack(FlagSet<eUndoStackOptions> options)
{
	m_Options = options;
}

void rageam::UndoStack::Clear()
{
	m_History.Clear();
	m_HistoryPoint = 0;
	m_SavePoint = 0;
}

void rageam::UndoStack::Redo()
{
	AM_ASSERT(CanRedo(), "UndoStack::Redo() -> We can't go into the future!");

	auto& undoable = m_History[m_HistoryPoint];
	for (int i = 0; i < undoable->m_GroupSize; i++)
	{
		m_History[m_HistoryPoint]->Do();
		m_HistoryPoint++;
	}
}

void rageam::UndoStack::Undo()
{
	AM_ASSERT(CanUndo(), "UndoStack::Undo() -> History is empty!");

	auto& undoable = m_History[m_HistoryPoint - 1];
	for (int i = 0; i < undoable->m_GroupSize; i++)
	{
		m_History[m_HistoryPoint - 1]->Undo();
		m_HistoryPoint--;
	}
}

void rageam::UndoStack::BeginGroup()
{
	AM_ASSERTS(m_GroupStartIndex == -1);
	m_GroupStartIndex = m_HistoryPoint;
}

void rageam::UndoStack::EndGroup()
{
	AM_ASSERTS(m_GroupStartIndex != -1);
	int groupSize = m_HistoryPoint - m_GroupStartIndex;
	if (groupSize >= 0) // Check if any action was added
	{
		// Link both sides for Redo/Undo
		m_History[m_HistoryPoint - 1]->m_GroupSize = groupSize;
		m_History[(u16)m_GroupStartIndex]->m_GroupSize = groupSize;
	}
	m_GroupStartIndex = -1;
}

void rageam::UndoStack::Add(IUndoable* action)
{
	// We can't edit stack in group mode...
	if (m_GroupStartIndex == -1 && m_History.GetSize() == MAX_HISTORY_SIZE)
	{
		m_History.RemoveFirst();
		m_HistoryPoint--;
	}

	bool eraseHistory = m_Options.IsSet(UNDO_CONTEXT_HARD_REDO);

	action->Do();

	if (eraseHistory)
	{
		// Anything that was undone is erased when new action is pushed
		m_History.Resize(m_HistoryPoint);
		m_History.Construct(action);
	}
	else
	{
		m_History.EmplaceAt(m_HistoryPoint, amUniquePtr<IUndoable>(action));
	}

	m_HistoryPoint++;
}

void rageam::UndoStack::AddFn(UndoableFn::TFn doFn, UndoableFn::TFn undoFn)
{
	Add(new UndoableFn(std::move(doFn), std::move(undoFn)));
}
