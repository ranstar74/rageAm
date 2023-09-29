#ifdef AM_UNIT_TESTS

#include "CppUnitTest.h"
#include "rage/atl/set.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace rage;

namespace Microsoft::VisualStudio::CppUnitTestFramework
{
	template<> std::wstring ToString<>(const atSet<int>& set)
	{
		std::wstring string;

		for (int value : set)
		{
			string += L" ";
			string += std::to_wstring(value);
		}
		string += L" ";

		return string;
	}

	template<> std::wstring ToString<>(const atSetIterator<int>& it)
	{
		if (!it.HasValue())
			return L"None";

		return String::FormatTemp(L"{ %u }", it.GetValue());
	}
}

namespace unit_testing
{
	TEST_CLASS(atSetTests)
	{
	public:
		TEST_METHOD(VerifyIterator)
		{
			atSet<int> set;
			atSet<int> iteratedSet;
			set.InitAndAllocate(17, false);

			for (int i = 0; i < 4000; i++)
				set.Insert(i);

			for (int i : set)
				iteratedSet.Insert(i);

			Assert::AreEqual(set, iteratedSet);
		}

		TEST_METHOD(VerifyDestructor)
		{
			sysMemAllocator* allocator = GetAllocator(ALLOC_TYPE_GENERAL);
			u64 memoryBefore = allocator->GetMemoryUsed();
			{
				atSet<std::shared_ptr<int[]>> set;

				auto array1 = std::make_shared<int[]>(1000);
				auto array2 = std::make_shared<int[]>(1000);
				auto array3 = std::make_shared<int[]>(1000);

				set.InsertAt(1, array1);
				set.InsertAt(2, array2);
				set.InsertAt(3, array3);
			}
			u64 memoryAfter = allocator->GetMemoryUsed();

			Assert::AreEqual(memoryBefore, memoryAfter);
		}

		TEST_METHOD(VerifyDestructor2)
		{
			sysMemAllocator* allocator = GetAllocator(ALLOC_TYPE_GENERAL);
			u64 memoryBefore = allocator->GetMemoryUsed();
			{
				atSet<std::shared_ptr<int[]>> set;

				auto array1 = std::make_shared<int[]>(1000);
				auto array2 = std::make_shared<int[]>(1000);
				{
					auto array3 = std::make_shared<int[]>(1000);
					set.InsertAt(1, array3);
				}
				auto array3 = set.GetAt(1);

				set.InsertAt(1, array1);
				set.InsertAt(1, array2);
				set.InsertAt(2, array1);
				set.InsertAt(2, array1);
				set.InsertAt(3, array2);
				set.InsertAt(3, array1);
				set.InsertAt(1, array3);
			}
			u64 memoryAfter = allocator->GetMemoryUsed();

			allocator->SanityCheck();

			Assert::AreEqual(memoryBefore, memoryAfter);
		}

		// Create large set and destroy it
		TEST_METHOD(VerifyCreateAndDestroyNoLeaks)
		{
			sysMemAllocator* allocator = GetAllocator(ALLOC_TYPE_GENERAL);
			u64 memoryBefore = allocator->GetMemoryUsed();
			{
				atSet<int> set;

				for (u16 i = 0; i < 30000; i++)
					set.Insert(i); // Set will grow multiple times

				set.Destruct();

				set.InitAndAllocate(10000, false);
				for (u16 i = 0; i < 30000; i++)
					set.Insert(i); // No growing
			}
			u64 memoryAfter = allocator->GetMemoryUsed();

			allocator->SanityCheck();

			Assert::AreEqual(memoryBefore, memoryAfter);
		}

		// Compares two equal large sets
		TEST_METHOD(VerifyComparison)
		{
			atSet<int> a;
			atSet<int> b;

			a.InitAndAllocate(10000, false);
			b.InitAndAllocate(10000, false);

			for (int i = 0; i < 10000; i++)
				a.Insert(i);
			for (int i = 9999; i >= 0; i--) // Different order because in set order shouldn't matter
				b.Insert(i);

			Assert::AreEqual(a, b);
		}

		// Verifies that a set initialized with multiple unique values will contain only distinct set of those values
		TEST_METHOD(VerifyInitializerListComparison)
		{
			atSet input = { 5, 5, 10, 2, 2, 3, 6, 6 };
			atSet expected = { 5, 10, 2, 3, 6 };

			Assert::AreEqual(expected, input);
		}

		// Same as VerifyInitializerListComparison but comparison list order is random
		TEST_METHOD(VerifyInitializerListUnorderedComparison)
		{
			atSet input = { 5, 5, 10, 2, 2, 3, 6, 6 };
			atSet expected = { 6, 3, 5, 10, 2 };

			Assert::AreEqual(expected, input);
		}

		// Tries to get iterator on value in set
		TEST_METHOD(VerifyIteratorFind)
		{
			atSet input = { 5, 10, 6 };

			auto it = input.Find(10);

			Assert::AreNotEqual(input.end(), it);
		}

		// Tries to get iterator on value in set and remove slot it points to
		TEST_METHOD(VerifyIteratorFindAndRemove)
		{
			atSet input = { 5, 10, 6 };
			atSet expected = { 5, 6 };

			auto it = input.Find(10);
			input.RemoveAtIterator(it);

			Assert::AreEqual(expected, input);
		}

		// Copies set and verifies if it is actually equal to initial one
		TEST_METHOD(VerifyCopying)
		{
			atSet input =
			{
				10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
				24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44,
			};

			atSet copy = input; // NOLINT(performance-unnecessary-copy-initialization)

			Assert::AreEqual(input, copy);
		}

		// Resizes set and verifies that it contains the same set of values
		TEST_METHOD(VerifyResize)
		{
			atSet<int> input;
			input.InitAndAllocate(3, false); // Don't allow to alter bucket count
			// This will create huge amount of bucket collisions so we can test that separate chaining
			// linked lists moved into new buckets properly too
			for (int i = 0; i < 100; i++)
				input.Insert(i);

			atSet resized = input;
			resized.Resize(500);

			Assert::AreEqual(input, resized);
		}
	};
}
#endif
