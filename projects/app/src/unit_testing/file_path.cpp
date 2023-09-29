#ifdef AM_UNIT_TESTS

#include "CppUnitTest.h"
#include "am/file/path.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace unit_testing
{
	using namespace rageam::file;

	TEST_CLASS(FilePathTests)
	{
	public:
		TEST_METHOD(VerifyJoin)
		{
			Path path = "dir";
			path /= "home";
			path /= "files";
			
			Assert::AreEqual("dir\\home\\files", path.GetCStr());
		}

		TEST_METHOD(VerifyAppend)
		{
			Path path = "GTA5";
			path += ".exe";

			Assert::AreEqual("GTA5.exe", path.GetCStr());
		}

		TEST_METHOD(VerifyGetFileExtension)
		{
			Path path = "RockstarGames/GrandTheftAutoV/GTA5.exe";
			Path extension = path.GetExtension();

			Assert::AreEqual("exe", extension.GetCStr());
		}

		TEST_METHOD(VerifyGetFileName)
		{
			Path path = "RockstarGames/GrandTheftAutoV/GTA5.exe";
			Path name = path.GetFileName();

			Assert::AreEqual("GTA5.exe", name.GetCStr());
		}

		TEST_METHOD(VerifyGetFileNameWithoutExtension)
		{
			Path path = "RockstarGames/GrandTheftAutoV/GTA5.exe";
			Path name = path.GetFileNameWithoutExtension();

			Assert::AreEqual("GTA5", name.GetCStr());
		}

		TEST_METHOD(VerifyGetFilePathWithoutExtension)
		{
			Path path = "RockstarGames/GrandTheftAutoV/GTA5.exe";
			Path filePath = path.GetFilePathWithoutExtension();

			Assert::AreEqual("RockstarGames/GrandTheftAutoV/GTA5", filePath.GetCStr());
		}

		TEST_METHOD(VerifyGetParentDirectory)
		{
			Path path = "RockstarGames/GrandTheftAutoV/GTA5.exe";
			Path parent = path.GetParentDirectory();

			Assert::AreEqual("RockstarGames/GrandTheftAutoV", parent.GetCStr());
		}

		TEST_METHOD(VerifyGetParentDirectoryWithLastSlash)
		{
			Path path = "RockstarGames/GrandTheftAutoV/";
			Path parent = path.GetParentDirectory();

			Assert::AreEqual("RockstarGames/GrandTheftAutoV", parent.GetCStr());
		}
	};
}
#endif
