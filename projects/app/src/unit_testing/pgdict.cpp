#ifdef AM_UNIT_TESTS

#include "CppUnitTest.h"
#include "rage/paging/template/dictionary.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace unit_testing
{
	using namespace rage;

	TEST_CLASS(pgDictionaryTests)
	{
		struct TestItem
		{
			int		Key;
			char	SomeData[512]{};
		};

	public:
		TEST_METHOD(VerifyAddGet)
		{
			sysMemAllocator* allocator = GetMultiAllocator();
			allocator->UpdateMemorySnapshots();
			u64 usedBefore = allocator->GetMemorySnapshot(0);
			{
				pgDictionary<TestItem> items;

				for (int i = 0; i < 1000; i++)
				{
					items.Insert(i, new TestItem(i));
				}

				for (int i = 999; i >= 0; i--)
				{
					Assert::AreEqual(items.IndexOf(i), i);
				}
			}
			u64 usedAfter = allocator->GetMemorySnapshot(0);
			Assert::AreEqual(usedBefore, usedAfter);
		}

		TEST_METHOD(VerifyRemove)
		{
			sysMemAllocator* allocator = GetMultiAllocator();
			allocator->UpdateMemorySnapshots();
			u64 usedBefore = allocator->GetMemorySnapshot(0);
			{
				pgDictionary<TestItem> items;

				items.Insert(5, new TestItem());
				items.Insert(3, new TestItem());
				items.Insert(155, new TestItem());
				items.Insert(16, new TestItem());
				items.Insert(87, new TestItem());
				items.Insert(255, new TestItem());

				Assert::AreEqual(4, items.IndexOf(155));
				items.Remove(155);
				Assert::AreEqual(-1, items.IndexOf(155));
				Assert::AreEqual(2, items.IndexOf(16));

				items.Insert(87, new TestItem(99));
				Assert::AreEqual(99, items.Find(87)->Key);
			}
			u64 usedAfter = allocator->GetMemorySnapshot(0);
			Assert::AreEqual(usedBefore, usedAfter);
		}
	};
}
#endif
