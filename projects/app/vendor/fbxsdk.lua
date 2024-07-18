files { "fbxsdk/**.h", "fbxsdk/**.cpp" }
includedirs { "fbxsdk/include/" }

filter { "configurations:Debug" }
	libdirs { "fbxsdk/lib/debug/" }
filter { "configurations:Release" }
	libdirs { "fbxsdk/lib/release/" }
filter {}

links { "alembic-md" }
links { "libfbxsdk-md" }
links { "libxml2-md" }
links { "zlib-md" }

files { "fbxsdk/lib/libfbxsdk.dll" }
