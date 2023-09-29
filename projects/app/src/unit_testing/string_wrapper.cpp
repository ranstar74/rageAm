#ifdef AM_UNIT_TESTS

#include "CppUnitTest.h"
#include "rage/system/allocator.h"
#include "rage/atl/string.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace rage;

namespace unit_testing
{
	TEST_CLASS(StringWrapperTests)
	{
	public:
		TEST_METHOD(VerifyLength)
		{
			ImmutableString stra = "RAGE AM";
			ImmutableWString strw = L"RAGE AM";
			Assert::AreEqual(stra.Length(), 7u);
			Assert::AreEqual(strw.Length(), 7u);
		}

		TEST_METHOD(VerifyEquals)
		{
			ImmutableString stra = "Test";
			ImmutableWString strw = L"Test";
			Assert::IsTrue(stra.Equals("Test"));
			Assert::IsTrue(strw.Equals(L"Test"));
			Assert::IsTrue(stra.Equals("test", true));
			Assert::IsTrue(strw.Equals(L"test", true));
		}

		TEST_METHOD(VerifyIndexOf)
		{
			ImmutableString stra = "RAGE AM / @ . @ /";
			ImmutableWString strw = L"RAGE AM / @ . @ /";
			Assert::AreEqual(stra.IndexOf('@'), 10);
			Assert::AreEqual(strw.IndexOf('@'), 10);
			Assert::AreEqual(stra.IndexOf<'@', '.'>(), 10);
			Assert::AreEqual(strw.IndexOf<'@', '.'>(), 10);
		}

		TEST_METHOD(VerifyLastIndexOf)
		{
			ImmutableString stra = "RAGE AM / @ . @ /";
			ImmutableWString strw = L"RAGE AM / @ . @ /";
			Assert::AreEqual(stra.LastIndexOf('@'), 14);
			Assert::AreEqual(strw.LastIndexOf('@'), 14);
			Assert::AreEqual(stra.LastIndexOf<'@', '.'>(), 14);
			Assert::AreEqual(strw.LastIndexOf<'@', '.'>(), 14);
		}

		TEST_METHOD(VerifyStartsWith)
		{
			ImmutableString stra = "RAGE AM / @ . @ /";
			ImmutableWString strw = L"RAGE AM / @ . @ /";
			Assert::IsTrue(stra.StartsWith("RAGE"));
			Assert::IsTrue(strw.StartsWith(L"RAGE"));
			Assert::IsTrue(stra.StartsWith("rAgE aM", true));
			Assert::IsTrue(strw.StartsWith(L"rAgE aM", true));
		}

		TEST_METHOD(VerifyEndsWith)
		{
			ImmutableString stra = "RAGE AM / @ . @ / SomeString";
			ImmutableWString strw = L"RAGE AM / @ . @ / SomeString";
			Assert::IsTrue(stra.EndsWith("SomeString"));
			Assert::IsTrue(strw.EndsWith(L"SomeString"));
			Assert::IsTrue(stra.EndsWith("SOMEsTrIng", true));
			Assert::IsTrue(strw.EndsWith(L"SOMEsTrIng", true));
		}

		TEST_METHOD(VerifySubstring)
		{
			ImmutableString stra = "RAGE AM";
			ImmutableWString strw = L"RAGE AM";
			Assert::IsTrue(stra.Substring(5).Equals("AM"));
			Assert::IsTrue(strw.Substring(5).Equals(L"AM"));
		}
	};
}
#endif
