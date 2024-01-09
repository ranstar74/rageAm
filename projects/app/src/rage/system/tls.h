#pragma once

namespace rage
{
#ifdef AM_INTEGRATED
	template<typename T>
	class ThreadLocal
	{
		int m_Offset = -1;

	public:
		ThreadLocal() = default;
		ThreadLocal(int offset) : ThreadLocal() { SetOffset(offset); }
		ThreadLocal(const ThreadLocal&) = delete;
		ThreadLocal(ThreadLocal&&) noexcept = delete;

		void SetOffset(int offset)
		{
			m_Offset = offset;
		}

		T& Get()
		{
			AM_ASSERT(m_Offset != -1, "ThreadLocal::Get() -> Offset was not set!");
			u64 tls = *(u64*)__readgsqword(0x58);
			return *(T*)(tls + m_Offset);
		}

		void Set(const T& value)
		{
			Get() = value;
		}

		operator T& () { return Get(); }

		ThreadLocal& operator=(const T& value) { Get() = value; return *this; }
		ThreadLocal& operator=(const ThreadLocal&) = delete;
		ThreadLocal& operator=(ThreadLocal&&) noexcept = delete;
	};
#endif

#ifdef AM_INTEGRATED
#define AM_TL(type) ThreadLocal<type>
#else
#define AM_TL(type) thread_local type
#endif
}
