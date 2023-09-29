#include "worker.h"

#include <utility>

#include "am/system/timer.h"

HANDLE rageam::BackgroundWorker::sm_ThreadPool[MAX_BACKGROUND_THREADS];
rage::atArray<amUniquePtr<rageam::BackgroundWorker::BackgroundJob>> rageam::BackgroundWorker::sm_Jobs;
std::mutex rageam::BackgroundWorker::sm_Mutex;
std::condition_variable rageam::BackgroundWorker::sm_Condition;
rageam::Event<rageam::TaskFinishInfo> rageam::BackgroundWorker::OnTaskFinished;

DWORD rageam::BackgroundWorker::ThreadProc(LPVOID lpParam)
{
	u64 workerId = u64(lpParam);

	// Add thread name so it can be seen in debugger
	{
		wchar_t nameBuffer[64];
		swprintf_s(nameBuffer, 64, L"[RAGEAM] Background Worker %llu", workerId);
		(void)SetThreadDescription(GetCurrentThread(), nameBuffer);
	}

	while (!sm_WeAreClosing)
	{
		amUniquePtr<BackgroundJob> job;
		{
			std::unique_lock lock(sm_Mutex);
			sm_Condition.wait(lock, []
				{
					return sm_Jobs.Any() || sm_WeAreClosing;
				});

			if (sm_WeAreClosing)
				break;

			job = std::move(sm_Jobs.First());
			sm_Jobs.RemoveAt(0);
		}

		Timer timer;

		timer.Start();
		job->GetTask()->m_State = TASK_STATE_RUNNING;
		bool success = job->GetLambda()();
		timer.Stop();
		job->GetTask()->m_State = success ? TASK_STATE_SUCCESS : TASK_STATE_FAILED;

		wchar_t buffer[256];
		swprintf_s(buffer, 256, L"[%ls] -> %hs, %llu ms", job->GetName(), success ? "Done" : "Failed", timer.GetElapsedMilliseconds());

		AM_DEBUGF(L"BackgroundWorker -> (WorkerID: %llu) -> %s", workerId, buffer);
		
		if (TaskCallback)
			TaskCallback(buffer);
	}
	return 0;
}

void rageam::BackgroundWorker::Init()
{
	for (u64 i = 0; i < MAX_BACKGROUND_THREADS; i++)
	{
		HANDLE thread = CreateThread(NULL, 0, ThreadProc, LPVOID(i), 0, NULL);
		AM_ASSERT(thread, "BackgroundWorker::Init() -> Failed to create thread, last error: %x", GetLastError());
		sm_ThreadPool[i] = thread;
	}
	sm_Initialized = true;
}

amPtr<rageam::BackgroundTask> rageam::BackgroundWorker::RunVA(const TLambda& lambda, ConstWString fmt, va_list args)
{
	std::unique_lock lock(sm_Mutex);
	if (!sm_Initialized)
		Init();

	amPtr<BackgroundTask> task = std::make_shared<BackgroundTask>();
	task->m_State = TASK_STATE_PENDING;

	wchar_t buffer[256];
	vswprintf_s(buffer, 256, fmt, args);

	sm_Jobs.Construct(new BackgroundJob(task, lambda, buffer));

	sm_Condition.notify_one();
	return task;
}

void rageam::BackgroundWorker::Shutdown()
{
	sm_WeAreClosing = true;
	sm_Condition.notify_all();

	for (HANDLE thread : sm_ThreadPool)
	{
		WaitForSingleObject(thread, INFINITE);
		CloseHandle(thread);
	}
	sm_Jobs.Destroy();
}

amPtr<rageam::BackgroundTask> rageam::BackgroundWorker::Run(const TLambda& lambda, ConstString fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	amPtr<BackgroundTask> result = RunVA(lambda, String::ToWideTemp(fmt), args);
	va_end(args);
	return result;
}

amPtr<rageam::BackgroundTask> rageam::BackgroundWorker::Run(const TLambda& lambda, ConstWString fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	amPtr<BackgroundTask> result = RunVA(lambda, fmt, args);
	va_end(args);
	return result;
}

amPtr<rageam::BackgroundTask> rageam::BackgroundWorker::Run(const TLambda& lambda)
{
	return Run(lambda, L"Background task");
}

bool rageam::BackgroundWorker::WaitFor(const Tasks& tasks)
{
	bool success = true;
	for (amPtr<BackgroundTask>& task : tasks)
	{
		task->Wait();
		if (!task->IsSuccess())
			success = false;
	}
	return success;
}
