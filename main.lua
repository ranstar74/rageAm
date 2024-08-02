require 'vendor'

-- Build configurations
platform_integrated = "Integrated"
platform_standalone = "Standalone"
platform_unit_tests = "Unit Tests"

function default_config()
	language "C++"
	targetdir "bin/%{cfg.buildcfg}"

	cppdialect "C++20"
	architecture "x64"
	flags { "MultiProcessorCompile" }
	symbols "On"

	-- TODO: Only disable if tracy profiler is enabled
	-- Works terribl and incompatible with Tracy, disable it for now
	editandcontinue "Off"

	filter { "options:avx2" }
		vectorextensions "AVX2"
		defines { "AM_IMAGE_USE_AVX2" }

	filter { "not options:avx2" }
		vectorextensions "SSE2"

	filter { "configurations:Debug" }
		defines { "DEBUG" }
		intrinsics "On"
		
	filter { "configurations:Release" }
		defines { "NDEBUG" }
		optimize "Speed"
		intrinsics "On"

	filter {}
end

function quote_path(path)
	return '\"' .. path .. '\"'
end

-- Adds launcher cmd command to eject DLL on pre build and inject it after build
function add_launcher_events(build_dir)
	-- Make sure all paths are quoted!
	build_dir = os.realpath(build_dir)
	
	local launcher = quote_path(build_dir .. "Launcher.exe")
	local rageAm = quote_path(build_dir .. "rageAm.dll")
	
	local exeName = _OPTIONS["exe"]
	local base_command = launcher .. " -exe " .. exeName .. " -dll " .. rageAm
	prebuildcommands { base_command .. " -unload" }
	buildcommands { base_command .. " -load" }
	
	-- Hack to make visual studio execute build command even if there's nothing to build.
	-- It will execute command if file not found (meaning everytime). Please don't put GTAVI in compile directory.
	buildoutputs { "GTAVI.exe" }
end

newoption {
   trigger = "nostacksymbols",
   description = "Disables .pdb symbols in stack trace."
}
newoption {
	trigger = "avx2",
	description = "Enables AVX2 SIMD instruction set.",
}
newoption {
	trigger = "exe",
	description = "Name of game executable to inject DLL in.",
	default = "GTA5.exe",
}
newoption {
	trigger = "easyprofiler",
	description = "Enables EasyProfiler.",
}
newoption { 
	trigger = "gamebuild",
	description = "Target build version of the game",
}
newoption { 
	trigger = "testbed",
	description = "Enables testbed UI, only for development.",
}
newoption { 
	trigger = "sovietkeys",
	description = "Internal use.",
}

workspace "rageAm"
	configurations { "Debug", "Release" }
	
	platforms
	{ 
		platform_integrated,
		platform_standalone,
		platform_unit_tests,
	}
	
	location "projects"

project "Launcher"
	kind "ConsoleApp"
	default_config()
	
	location "projects/launcher"

	files 
	{ 
		"projects/launcher/src/**.h", 
		"projects/launcher/src/**.cpp" 
	}

function setup_game_build_version()
	-- 2699.16 Release (Built from source code)
	filter { "options:gamebuild=2699_16_RELEASE_NO_OPT" }
		defines { "APP_BUILD_ID=1" }
		defines { "APP_BUILD_2699_16_RELEASE=1" }
		defines { "APP_BUILD_2699_16_RELEASE_NO_OPT=1" } -- Includes APP_BUILD_2699_16_RELEASE, but higher priority
		defines { "APP_BUILD_STRING=\"2699.16 Dev\"" }
	
	-- 2699.16 Master
	filter { "options:gamebuild=2699_16" }
		defines { "APP_BUILD_ID=1" }
		defines { "APP_BUILD_2699_16=1" }
		defines { "APP_BUILD_STRING=\"2699.16 Master\"" }
	
	-- 3095.0
	filter { "options:gamebuild=3095_0" }
		defines { "APP_BUILD_ID=2" }
		defines { "APP_BUILD_3095_0=1" }
		defines { "USE_RAGE_RTTI" } -- 2802+
		defines { "APP_BUILD_STRING=\"3095.0 Master\"" }
	
	-- 3179.0
	filter { "options:gamebuild=3179_0" }
		defines { "APP_BUILD_ID=3" }
		defines { "APP_BUILD_3179_0=1" }
		defines { "USE_RAGE_RTTI" } -- 2802+
		defines { "APP_BUILD_STRING=\"3179.0 Master\"" }
	
	-- 3258.0
	filter { "options:gamebuild=3258_0" }
		defines { "APP_BUILD_ID=4" }
		defines { "APP_BUILD_3258_0=1" }
		defines { "USE_RAGE_RTTI" } -- 2802+
		defines { "APP_BUILD_STRING=\"3258.0 Master\"" }
		
	-- 3274.0
	filter { "options:gamebuild=3274_0" }
		defines { "APP_BUILD_ID=5" }
		defines { "APP_BUILD_3274_0=1" }
		defines { "USE_RAGE_RTTI" } -- 2802+
		defines { "APP_BUILD_STRING=\"3274.0 Master\"" }
	
	-- Defines for comparison
	defines { "APP_BUILD_ID_2699_16=1" }
	defines { "APP_BUILD_ID_3095_0=2" }
	defines { "APP_BUILD_ID_3179_0=3" }
	defines { "APP_BUILD_ID_3258_0=4" }
	defines { "APP_BUILD_ID_3274_0=5" }
	
	filter {}
end

project "rageAm"
	debugdir "bin/%{cfg.buildcfg}" -- Work directory

	local configName = ""
	filter { "configurations:Debug" }
		configName = "Debug"
	filter { "configurations:Release" }
		configName = "Release"
	filter()
	
	-- Integrated: DLL
	filter { "platforms:" .. platform_integrated }
		kind "SharedLib"
		defines { "AM_INTEGRATED" }
		add_launcher_events("bin/%{cfg.buildcfg}" .. "/")
		setup_game_build_version()
	-- Standalone: EXE
	filter { "platforms:" .. platform_standalone }
		kind "ConsoleApp"
		defines { "AM_STANDALONE" }
	-- Unit Tests: DLL
	filter { "platforms:" .. platform_unit_tests }
		kind "SharedLib"
		defines { "AM_STANDALONE" }
		defines { "AM_UNIT_TESTS" }

	filter { "not options:nostacksymbols" }
		defines { "AM_STACKTRACE_SYMBOLS" }

	filter { "options:testbed" }
		defines { "AM_TESTBED" }
	
	filter {}
	
	location "projects/app"
	default_config()
	dpiawareness "HighPerMonitor"

	files 
	{ 
		"projects/app/src/**.h", 
		"projects/app/src/**.cpp", 
		"projects/app/src/**.hint", 
		"projects/app/src/**.natvis",
		
		"projects/app/resources/*.*"
	}
	
	includedirs { "projects/app/src" }

	-- TODO: Can we rewrite this mess better?
	defines { "IMGUI_USER_CONFIG=" .. "\"" .. (os.realpath("projects/app/src/am/ui/imgui_config.h")) .. "\"" }
	defines { "AM_DEFAULT_DATA_DIR=" .. "LR\"(" .. (os.realpath("data")) .. ")\"" }
	defines { "AM_DATA_DIR=L\"data\"" }
	
	-- Keep zlib-ng for now because it is the fastest
	-- defines { "AM_ZLIB" }
	-- include_vendors { "zlib" };
	-- defines { "AM_MINIZ" }
	-- include_vendors { "miniz" }
	defines { "AM_ZLIB_NG" }
	include_vendors { "zlib-ng" };
	
	include_vendors {
		"slgui", -- ImGui
		"implot",
		"tinyxml2",
		"minhook",
		"freetype",
		"magic_enum",
		"cgltf",
		"ufbx",
		"fbxsdk",
		"hwbreakpoint",
		"stb_image",
		"stb_sprintf",
		"bc7enc_rdo",
		"libwebp",
		"icbc",
		"lunasvg",
		"tracy",
	}
	links { "Comctl32.lib" } 	-- TaskDialog
	links { "dbghelp" } 		-- StackTrace
	links { "d3d11.lib" }
	links { "Shlwapi.lib" } 	-- PathMatchSpecA
	links { "dxguid.lib" } 		-- Reflection
	
	filter "files:**.natvis"
		buildaction "Natvis"
	filter{}
	
	defines { "NOMINMAX" }

	-- GRPC + Protobuf
	include_vendors 
	{
		"grpc",
	}
	files
	{
		"projects/app/src/protoc/**.*",
	}
	-- https://stackoverflow.com/questions/21399650/cannot-include-both-files-winsock2-windows-h
	defines { "_WINSOCKAPI_" }
	includedirs { "protos/include_cpp" }

	-- Easy Profiler
	include_vendors {
		"easy_profiler",
	}
	defines { "EASY_PROFILER_VERSION_MAJOR=2" }
	defines { "EASY_PROFILER_VERSION_MINOR=1" }
	defines { "EASY_PROFILER_VERSION_PATCH=0" }
	defines { "EASY_PRODUCT_VERSION_STRING=\"2.1.0\"" }
	defines { "_BUILD_PROFILER" }
	filter { "options:easyprofiler" }
		defines { "AM_EASYPROFILER" }
		defines { "BUILD_WITH_EASY_PROFILER" }
	filter {}

	-- Encryption
	filter { "options:sovietkeys" }
		defines { "AM_SOVIET_KEYS" }
		include_vendors { "gta5aes" }
	filter {}
