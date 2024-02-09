require 'vendor'

function default_config()
	language "C++"
	targetdir "bin/%{cfg.buildcfg}"

	cppdialect "C++20"
	architecture "x64"
	flags { "MultiProcessorCompile" }
	symbols "On"

	filter { "options:avx2" }
		vectorextensions "AVX2"
		defines { "AM_IMAGE_USE_AVX2" }
	
	filter { "not options:avx2" }
		vectorextensions "SSE2"

	filter "configurations:Debug"
		defines { "DEBUG" }
		intrinsics "On"

	filter "configurations:Release"
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
   trigger = "standalone",
   description = "Compile rageAm as command line offline resource compiler."
}

newoption {
   trigger = "unittests",
   description = "Enable microsoft native unit tests."
}

newoption {
   trigger = "nostacksymbols",
   description = "Disables .pdb symbols in stack trace."
}

newoption {
	trigger = "avx2",
	description = "Enables AVX2 SIMD instruction set.",
}

newoption {
	trigger = "integrated",
	description = "Enables game-specific hooks and integration components.",
}

newoption {
	trigger = "injected",
	description = "Compiles project as DLL to inject in running game instance.",
}

newoption {
	trigger = "exe",
	description = "Name of game executable to inject DLL in.",
	default = "GTA5.exe",
}

workspace "rageAm"
	configurations { "Debug", "Release" }
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

project "rageAm"
	debugdir "bin/%{cfg.buildcfg}" -- Work directory
	
	-- Unit Tests: DLL
	-- Standalone: EXE
	-- Integrated: DLL
	
	filter { "options:unittests" }
		kind "SharedLib"
		defines { "AM_STANDALONE" }
		defines { "AM_UNIT_TESTS" }

	filter { "options:standalone" }
		kind "ConsoleApp"
		defines { "AM_STANDALONE" }
	
	filter { "options:integrated" }
		defines { "AM_INTEGRATED" }

	filter { "options:injected" }
		kind "SharedLib"
		add_launcher_events("bin/%{cfg.buildcfg}" .. "/")
	
	filter { "not options:nostacksymbols" }
		defines { "AM_STACKTRACE_SYMBOLS" }

	filter{}
	
	location "projects/app"

	default_config()
	
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
	
	include_vendors {
		"slgui", -- ImGui
		"imguizmo",
		"implot",
		"tinyxml2",
		"minhook",
		"freetype",
		"zlib",
		"magic_enum",
		"cgltf",
		"ufbx",
		"hwbreakpoint",
		"stb_image",
		"bc7enc_rdo",
		"libwebp",
		"icbc",
		"lunasvg",
	}
	links { "Comctl32.lib" } 	-- TaskDialog
	links { "dbghelp" } 		-- StackTrace
	links { "d3d11.lib" }
	links { "Shlwapi.lib" } 	-- PathMatchSpecA
	links { "dxguid.lib" } 		-- Reflection
	
	filter "files:**.natvis"
		buildaction "Natvis"
	filter{}
	
	dpiawareness "HighPerMonitor"

	defines { "NOMINMAX" }

	defines { "APP_BUILD_2699_16=1" }
	defines { "APP_BUILD_2699_16_FINAL=0" }
	defines { "APP_BUILD_2699_16_RELEASE=1" }
	defines { "APP_BUILD_2699_16_RELEASE_NO_OPT=1" } -- Includes APP_BUILD_2699_16_RELEASE, but higher priority

	-- Profiler
	include_vendors {
		"easy_profiler",
	}
	defines { "BUILD_WITH_EASY_PROFILER" }
	defines { "_BUILD_PROFILER" }
	defines { "EASY_PROFILER_VERSION_MAJOR=2" }
	defines { "EASY_PROFILER_VERSION_MINOR=1" }
	defines { "EASY_PROFILER_VERSION_PATCH=0" }
	defines { "EASY_PRODUCT_VERSION_STRING=\"2.1.0\"" }
