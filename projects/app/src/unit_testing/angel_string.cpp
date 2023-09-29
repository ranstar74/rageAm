#ifdef AM_UNIT_TESTS

#include "CppUnitTest.h"
#include "rage/system/allocator.h"
#include "rage/system/systemheap.h"
#include "rage/atl/string.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace rage;

namespace unit_testing
{
	TEST_CLASS(atStringTests)
	{
	public:
		TEST_METHOD(VerifyReplace)
		{
			atString msg = "Name: __name__, Time: __hour__:__minute__:__second__, Accepted: __accepted__";
			msg = msg.Replace("__name__", "Rage Am");
			msg = msg.Replace("__hour__", "1");
			msg = msg.Replace("__minute__", "21");
			msg = msg.Replace("__second__", "00");
			msg = msg.Replace("__accepted__", "True");
			Assert::AreEqual(msg.GetCStr(), "Name: Rage Am, Time: 1:21:00, Accepted: True");
		}

		TEST_METHOD(VerifyReverse)
		{
			atString msg = "mirror";
			Assert::AreEqual(msg.Reverse().GetCStr(), "rorrim");
		}

		TEST_METHOD(VerifyFormat)
		{
			atString str;
			str.AppendFormat("%u", 255);
			Assert::AreEqual(str.GetCStr(), "255");
		}

		TEST_METHOD(VerifyHasNoMemoryLeaks)
		{
			sysMemAllocator* allocator = GetMultiAllocator();

			allocator->UpdateMemorySnapshots();
			u64 usedBefore = allocator->GetMemorySnapshot(0);

			{
				// Do some crazy batshit things
				atString str = "Unit Testing!";
				str.Reserve(10000);
				str += "1";
				str += "555551111";
				str = str.Reverse();
				str = str.ToLowercase();
				str = nullptr;
				str = str.ToUppercase();
				str = str.Substring(1, 1);
				str = "QWERTY";
				str = "";
				str = nullptr;
				str.Shrink();
				str = "Hello!";
				str.Shrink();
			}

			{
				atString strArray[50];
				for (atString& str : strArray)
					str.AppendFormat("%s", "Hello Format!");
			}

			u64 usedAfter = allocator->GetMemorySnapshot(0);

			Assert::AreEqual(usedBefore, usedAfter);
		}
	};
}
#endif
