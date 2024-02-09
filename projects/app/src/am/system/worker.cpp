#include "worker.h"
#include "am/system/timer.h"

#include <utility>
#include <easy/profiler.h>

thread_local rage::atFixedArray<rageam::BackgroundWorker*, 8> rageam::BackgroundWorker::sm_Stack;

DWORD rageam::BackgroundWorker::ThreadProc(LPVOID lpParam)
{
	ThreadProcArg*     arg = static_cast<ThreadProcArg*>(lpParam);
	BackgroundWorker*  worker = arg->Instance;
	int				   workerID = arg->WorkerID;
	delete arg;
	arg = nullptr;

	// Add thread name so it can be seen in debugger
	{
		wchar_t nameBuffer[64];
		swprintf_s(nameBuffer, 64, L"[RAGEAM] Worker %hs [%i]", worker->m_Name, workerID);
		(void) SetThreadDescription(GetCurrentThread(), nameBuffer);

		EASY_THREAD(String::ToAnsiTemp(nameBuffer));
	}

	while (!worker->m_WeAreClosing)
	{
		// Wait for the next job
		amUPtr<BackgroundJob> job;
		{
			std::unique_lock lock(worker->m_Mutex);
			worker->m_Condition.wait(lock, [&]
				{
					return worker->m_Jobs.Any() || worker->m_WeAreClosing;
				});

			if (worker->m_WeAreClosing)
				break;

			job = std::move(worker->m_Jobs.First());
			worker->m_Jobs.RemoveAt(0);
		}

		Timer timer = Timer::StartNew();
		auto& task = job->GetTask();
		task->m_WorkerID = workerID;
		task->m_State = TASK_STATE_RUNNING;
		bool success = job->GetLambda()();
		task->m_Result = std::move(tl_Result);
		task->m_State = success ? TASK_STATE_SUCCESS : TASK_STATE_FAILED;
		timer.Stop();

		wchar_t buffer[256];
		if (String::IsNullOrEmpty(job->GetName()))
			swprintf_s(buffer, 256, L"%hs, %llu ms", success ? "OK" : "FAIL", timer.GetElapsedMilliseconds());
		else
			swprintf_s(buffer, 256, L"[%ls] %hs, %llu ms", job->GetName(), success ? "OK" : "FAIL", timer.GetElapsedMilliseconds());

#ifdef WORKER_ENABLE_LOGGING
		 AM_TRACEF(L"[W: %hs] wID:%i, %s", worker->m_Name, workerID, buffer);
#endif

		 if (worker->TaskCallback)
		 	worker->TaskCallback(buffer);
	}
	return 0;
}

amPtr<rageam::BackgroundTask> rageam::BackgroundWorker::RunVA(const TLambda& lambda, ConstWString fmt, va_list args)
{
	std::unique_lock lock(m_Mutex);

	amPtr<BackgroundTask> task = std::make_shared<BackgroundTask>();
	task->m_State = TASK_STATE_PENDING;

	wchar_t buffer[256];
	vswprintf_s(buffer, 256, fmt, args);

	m_Jobs.Construct(new BackgroundJob(task, lambda, buffer));

	m_Condition.notify_one();
	return task;
}

rageam::BackgroundWorker::BackgroundWorker(ConstString name, int threadCount)
{
	m_Name = name;
	m_ThreadPool.Resize(threadCount);
	for (u64 i = 0; i < threadCount; i++)
	{
		ThreadProcArg* arg = new ThreadProcArg(this, i);
		HANDLE thread = CreateThread(NULL, 0, ThreadProc, arg, 0, NULL);
		AM_ASSERT(thread, "BackgroundWorker::Init() -> Failed to create thread, last error: %x", GetLastError());
		m_ThreadPool[i] = thread;
	}
}

rageam::BackgroundWorker::~BackgroundWorker()
{
	m_WeAreClosing = true;
	m_Condition.notify_all();

	for (HANDLE thread : m_ThreadPool)
	{
		WaitForSingleObject(thread, INFINITE);
		CloseHandle(thread);
	}
}

amPtr<rageam::BackgroundTask> rageam::BackgroundWorker::Run(const TLambda& lambda, ConstString fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	amPtr<BackgroundTask> result = GetInstance()->RunVA(lambda, String::ToWideTemp(fmt), args);
	va_end(args);
	return result;
}

amPtr<rageam::BackgroundTask> rageam::BackgroundWorker::Run(const TLambda& lambda, ConstWString fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	amPtr<BackgroundTask> result = GetInstance()->RunVA(lambda, fmt, args);
	va_end(args);
	return result;
}

amPtr<rageam::BackgroundTask> rageam::BackgroundWorker::Run(const TLambda& lambda)
{
	return Run(lambda, L"");
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
