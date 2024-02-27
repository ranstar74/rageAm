#pragma once

#include "helpers/com.h"
#include "resource.h"
#include "compiler/compiler.h"
#include "compiler/snapshotallocator.h"

namespace rage
{
	// In rage terms it seems to be called rage::datOwner

	template<typename T>
	class pgPtrBase
	{
	protected:
		T* m_Pointer;

	public:
		// Automatic placing if in resource constructor
		pgPtrBase()
		{
			datResource* rsc = datResource::GetCurrent();
			if (rsc)
				*rsc >> m_Pointer;
			else
				m_Pointer = nullptr;
		}

		// Initialize with pointer
		pgPtrBase(T* ptr)
		{
			// Don't increment on initialization because rage uses default ref count 1
			m_Pointer = ptr;
		}

		// ReSharper disable once CppPossiblyUninitializedMember
		pgPtrBase(const datResource& rsc)
		{
			rsc >> m_Pointer;
		}

		pgPtrBase(pgPtrBase&& other) noexcept : pgPtrBase()
		{
			std::swap(m_Pointer, other.m_Pointer);
		}

		// Don't use! Reserved for resource compiler.
		// This makes identical smart pointer copy without accounting refs.
		void Snapshot(const pgPtrBase& other, bool physical = false)
		{
			pgSnapshotAllocator* allocator =
				physical ? pgRscCompiler::GetPhysicalAllocator() : pgRscCompiler::GetVirtualAllocator();

			// We use snapshot function to resolve derived classes
			constexpr bool hasSnapshotFn = requires(T t)
			{
				T::Snapshot(allocator, &t);
			};

			if constexpr (hasSnapshotFn)
			{
				m_Pointer = static_cast<T*>(T::Snapshot(allocator, other.m_Pointer));
				allocator->AddRef(m_Pointer);
			}
			else if constexpr (std::is_abstract_v<T>)
			{
				AM_UNREACHABLE("Cannot be used on abstract classes");
			}
			else
			{
				if (other.m_Pointer != nullptr)
				{
					allocator->AllocateRef(m_Pointer);
					new (m_Pointer) T(*other.m_Pointer);
				}
			}
		}

		// Adds ref to internal pointer if compiling, otherwise doing nothing
		void AddCompilerRef(bool physical = false)
		{
			pgRscCompiler* compiler = pgRscCompiler::GetCurrent();
			if (!compiler)
				return;
			if (physical)
				pgRscCompiler::GetPhysicalAllocator()->AddRef(this->m_Pointer);
			else
				pgRscCompiler::GetVirtualAllocator()->AddRef(this->m_Pointer);
		}

		// Sets pointer value without deleting previous pointer
		// Used in rare cases... mostly to deal with legacy rage code
		// NOTE: Pointer has to be reset to old one before this smart pointer goes out of scope!
		void Set(T* ptr)
		{
			m_Pointer = ptr;
		}

		T* Get() const { return m_Pointer; }
		T& GetRef() { return *m_Pointer; }
		const T& GetRef() const { return *m_Pointer; }

		bool operator==(const T* ptr) const { return m_Pointer == ptr; }
		bool operator==(const pgPtrBase& other) const { return m_Pointer == other.m_Pointer; }

		T* operator->() const { return m_Pointer; }
		T& operator*() { return *m_Pointer; }
		const T& operator*() const { return *m_Pointer; }

		// Implicit bool cast operator to make if(ptr) construction work
		operator bool() const { return m_Pointer != nullptr; }
		bool IsNull() const { return m_Pointer == nullptr; }

		// Used by 'weak' pointers that share single pointer with something else,
		// but still use fixup code
		void SuppressDelete()
		{
			m_Pointer = nullptr;
		}
	};

	/**
	 * \brief Pointer with internal ref counter, for classes like rmcDrawable
	 */
	template<typename T>
	class pgCountedPtr : public pgPtrBase<T>
	{
		s16* m_RefCount = nullptr;

		void AddRef() const { if(m_RefCount) ++(*m_RefCount); }
		s16  Release() const { return --(*m_RefCount); }

	public:
		pgCountedPtr() = default;
		explicit pgCountedPtr(T* object)
		{
			this->m_Pointer = object;
			m_RefCount = new s16(1);
		}
		pgCountedPtr(const pgCountedPtr& other)
		{
			this->m_Pointer = other.m_Pointer;
			m_RefCount = other.m_RefCount;
			AddRef();
		}
		pgCountedPtr(pgCountedPtr&& other) noexcept
		{
			std::swap(this->m_Pointer, other.m_Pointer);
			std::swap(m_RefCount, other.m_RefCount);
		}
		~pgCountedPtr() { Reset(); }

		void Reset()
		{
			if (m_RefCount && Release() == 0)
			{
				delete this->m_Pointer;
				delete m_RefCount;
			}
			this->m_Pointer = nullptr;
			m_RefCount = nullptr;
		}

		s16 GetRefCount() const { return m_RefCount ? *m_RefCount : 0; }

		pgCountedPtr& operator=(const pgCountedPtr& other)
		{
			Reset();
			m_RefCount = other.m_RefCount;
			this->m_Pointer = other.m_Pointer;
			AddRef();
			return *this;
		}

		pgCountedPtr& operator=(pgCountedPtr&& other) noexcept
		{
			std::swap(this->m_Pointer, other.m_Pointer);
			std::swap(this->m_RefCount, other.m_RefCount);
			return *this;
		}

		pgCountedPtr& operator=(nullptr_t)
		{
			Reset();
			return *this;
		}
	};

	/**
	 * \brief Shared pointer holder for paged objects.
	 * T class must implement AddRef & Release functions.
	 */
	template<typename T>
	class pgPtr : public pgPtrBase<T>
	{
	public:
		using pgPtrBase<T>::pgPtrBase;

		explicit pgPtr(T* ptr) : pgPtrBase<T>(ptr)
		{

		}
		pgPtr(nullptr_t) : pgPtrBase<T>(nullptr)
		{
			
		}
		pgPtr(const pgPtr& other)
		{
			this->m_Pointer = other.m_Pointer;
			SAFE_ADDREF(this->m_Pointer);
		}

		~pgPtr()
		{
			SAFE_RELEASE(this->m_Pointer);
		}

		void Copy(const pgPtr& from)
		{
			if (from.m_Pointer == nullptr)
				return;
			this->m_Pointer = new T(*from.m_Pointer);
		}

		pgPtr& operator=(pgPtr&& other) noexcept
		{
			std::swap(this->m_Pointer, other.m_Pointer);
			return *this;
		}

		pgPtr& operator=(const pgPtr& other)  // NOLINT(bugprone-unhandled-self-assignment)
		{
			if (pgRscCompiler::GetCurrent())
			{
				this->Snapshot(other);
				return *this;
			}

			if (this->m_Pointer != other.m_Pointer)
			{
				SAFE_RELEASE(this->m_Pointer);
				this->m_Pointer = other.m_Pointer;
				SAFE_ADDREF(this->m_Pointer);
			}
			return *this;
		}

		pgPtr& operator=(nullptr_t)
		{
			SAFE_RELEASE(this->m_Pointer);
			this->m_Pointer = nullptr;
			return *this;
		}
	};

	/**
	 * \brief Unique (single reference) pointer holder for paged objects.
	 * No copying allowed, only ownership move.
	 */
	template<typename T>
	class pgUPtr : public pgPtrBase<T>
	{
	public:
		using pgPtrBase<T>::pgPtrBase;

		pgUPtr(T* ptr) : pgPtrBase<T>(ptr) {}
		pgUPtr(const pgUPtr& other)
		{
			if (pgRscCompiler::GetCurrent())
				this->Snapshot(other);
			else
				this->Copy(other);
		}
		pgUPtr(pgUPtr&& other) noexcept { std::swap(this->m_Pointer, other.m_Pointer); }
		~pgUPtr()
		{
			Delete();
		}

		void Copy(const pgUPtr& from)
		{
			if (from.m_Pointer == nullptr)
				return;
			this->m_Pointer = new T(*from.m_Pointer);
		}

		pgUPtr& operator=(pgUPtr&& other) noexcept
		{
			std::swap(this->m_Pointer, other.m_Pointer);
			return *this;
		}

		pgUPtr& operator=(const pgUPtr& other) = delete;

		void Delete()
		{
			delete this->m_Pointer;
			this->m_Pointer = nullptr;
		}
	};

	/**
	 * \brief Wrapper for C array of pgPtr's, not to be used with raw pointers!
	 */
	template<typename T>
	class pgCArray : public pgPtrBase<T>
	{
	public:
		pgCArray(T* elements)
		{
			this->m_Pointer = elements;
		}
		pgCArray() : pgPtrBase<T>(this->m_Pointer) // Prevent pgPtrBase from placing
		{
			// We don't use automatic place operator here (>>) because it would
			// place the first item of array and ignore the rest, PlaceItems must be used instead
			datResource* rsc = datResource::GetCurrent();
			if (rsc)
				rsc->Fixup(this->m_Pointer);
			else
				this->m_Pointer = nullptr;
		}
		~pgCArray()
		{
			// Default destructor for simple struct array that doesn't require special handling,
			// for anything else DestroyItems must be used
			rage_free(this->m_Pointer);
			this->m_Pointer = nullptr;
		}

		template<typename TSize>
		void Copy(const pgCArray& other, TSize elementCount)
		{
			size_t allocSize = sizeof T * elementCount;

			pgRscCompiler* compiler = pgRscCompiler::GetCurrent();
			if (compiler)
			{
				compiler->GetVirtualAllocator()->AllocateRef(this->m_Pointer, allocSize);
			}
			else
			{
				this->m_Pointer = reinterpret_cast<T*>(new char[allocSize]);
			}

			for (TSize i = 0; i < elementCount; ++i)
			{
				new (&this->m_Pointer[i]) T(other.m_Pointer[i]);
			}
		}

		// NOTE: Must be used instead of operator[]
		// C++ adds array size for 'non trivially destructible' types and passing
		// such pointer to delete will cause issues, we would use delete[]
		// here but that won't be compatible with R* structs
		// Remarks: Default constructor is invoked on every element
		template<typename TSize>
		void AllocItems(TSize elementCount)
		{
			AM_ASSERT(this->m_Pointer == nullptr, "pgCArray::AllocItems() -> Old items must be destructed first!");

			T* elements = (T*)rage_malloc(sizeof T * elementCount);
			for (size_t i = 0; i < elementCount; i++)
				new (elements + i) T(); // Placement new

			this->m_Pointer = elements;
		}

		template<typename TSize>
		void PlaceItems(const datResource& rsc, TSize elementCount)
		{
			// We don't fixup anything here because it will be done in pgPtr / pgUPtr
			for (TSize i = 0; i < elementCount; ++i)
			{
				rsc.Place(this->m_Pointer + i);
			}
		}

		// Manual destructor, must be called for collections that require destructor call for each item
		template<typename TSize>
		void DestroyItems(TSize elementCount)
		{
			for (TSize i = 0; i < elementCount; ++i)
			{
				this->m_Pointer[i].~T();
			}

			// We can't use 'delete' operator because it would
			// call destructor on the first element and we already did it
			rage_free(this->m_Pointer);
			this->m_Pointer = nullptr;
		}

		pgCArray& operator=(pgCArray& other) = delete;
		pgCArray& operator=(pgCArray&& other) noexcept
		{
			std::swap(this->m_Pointer, other.m_Pointer);
			return *this;
		}

		template<typename TSize>
		T& operator[](TSize index) { return this->m_Pointer[index]; }

		template<typename TSize>
		const T& operator[](TSize index) const { return this->m_Pointer[index]; }
	};
}
