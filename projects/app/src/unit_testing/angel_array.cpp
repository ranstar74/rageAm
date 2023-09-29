#ifdef AM_UNIT_TESTS

#include "CppUnitTest.h"
#include "rage/system/allocator.h"
#include "rage/system/systemheap.h"
#include "rage/atl/array.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace rage;

namespace unit_testing
{
	TEST_CLASS(atArrayTests)
	{
	public:
		TEST_METHOD(VerifyCreateAndDestroyNoLeaks1)
		{
			sysMemAllocator* allocator = GetAllocator(ALLOC_TYPE_GENERAL);
			u64 memoryBefore = allocator->GetMemoryUsed();
			{
				atArray<std::shared_ptr<int[]>> arr;
				arr.Resize(100);

				auto array1 = std::make_shared<int[]>(1000);
				auto array2 = std::make_shared<int[]>(1000);
				auto array3 = std::make_shared<int[]>(1000);

				arr.Insert(1, array1);
				arr.Insert(2, array2);
				arr.Insert(3, array3);
			}
			u64 memoryAfter = allocator->GetMemoryUsed();

			allocator->SanityCheck();

			Assert::AreEqual(memoryBefore, memoryAfter);
		}

		TEST_METHOD(VerifyCreateAndDestroyNoLeaks2)
		{
			sysMemAllocator* allocator = GetAllocator(ALLOC_TYPE_GENERAL);
			u64 memoryBefore = allocator->GetMemoryUsed();
			{
				atArray<std::shared_ptr<int[]>> arr;
				arr.Resize(100);

				auto array1 = std::make_shared<int[]>(1000);
				auto array2 = std::make_shared<int[]>(1000);
				{
					auto array3 = std::make_shared<int[]>(1000);
					arr.Insert(1, array3);
				}
				auto array3 = arr.Get(1);

				arr.Insert(1, array1);
				arr.Insert(1, array2);
				arr.Insert(2, array1);
				arr.Insert(2, array1);
				arr.Insert(3, array2);
				arr.Insert(3, array1);
				arr.Insert(1, array3);
			}
			u64 memoryAfter = allocator->GetMemoryUsed();

			allocator->SanityCheck();

			Assert::AreEqual(memoryBefore, memoryAfter);
		}
	};
}

#endif
