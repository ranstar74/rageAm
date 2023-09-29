files { "directxtk/include/**.h", "directxtk/include/**.cpp" }
includedirs { "directxtk/include/" }

libdirs { "directxtk/lib/" }

filter "configurations:Debug"
	links { "DirectXTK_Debug" }

filter "configurations:Release"
	links { "DirectXTK_Release" }

filter{}
