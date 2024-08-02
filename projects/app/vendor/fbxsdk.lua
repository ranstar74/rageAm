local sdkPath  = os.getenv("FBXSDK")

files { sdkPath .. "/**.h", sdkPath .. "/**.cpp" }
includedirs { sdkPath .. "/include/" }

filter { "configurations:Debug" }
	libdirs { sdkPath .. "/lib/x64/debug/" }
	files { sdkPath .. "/lib/x64/debug/libfbxsdk.dll" }
filter { "configurations:Release" }
	libdirs { sdkPath .. "/lib/x64/release/" }
	files { sdkPath .. "/lib/x64/release/libfbxsdk.dll" }
filter {}

links { "alembic-md" }
links { "libfbxsdk-md" }
links { "libxml2-md" }
links { "zlib-md" }
