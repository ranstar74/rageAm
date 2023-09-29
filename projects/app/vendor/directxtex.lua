vendor_files "directxtex/include/" {
	"DirectXTex.h",
	"DirectXTex.inl"
}

libdirs { "directxtex/lib/" }

filter "configurations:Debug"
	links { "DirectXTex_Debug" }

filter "configurations:Release"
	links { "DirectXTex_Release" }

filter{}
