require 'vendor'

function default_config()
	language "C++"
	targetdir "bin/%{cfg.buildcfg}"

	cppdialect "C++20"
	vectorextensions "AVX2"
	architecture "x64"
	flags { "MultiProcessorCompile" }

	symbols "On"
	filter "configurations:Debug"
		defines { "DEBUG" }

	filter "configurations:Release"
		defines { "NDEBUG" }
		optimize "Full"
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
	
	-- local scylla = quote_path(os.realpath("tools/scyllahide.dll"))
	local launcher = quote_path(build_dir .. "Launcher.exe")
	local rageAm = quote_path(build_dir .. "rageAm.dll")

	local base_command = launcher .. " -exe GTA5.exe -dll " .. rageAm -- .. " -scylla " .. scylla
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
	
	filter { "not options:standalone", "not options:unittests" }
		kind "SharedLib"
		add_launcher_events("bin/%{cfg.buildcfg}" .. "/")
		defines { "AM_INTEGRATED" }

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
	defines { "IMGUI_USER_CONFIG=" .. "\"" .. (os.realpath("projects/app/src/am/ui/imgui/config.h")) .. "\"" }
	defines { "AM_DEFAULT_DATA_DIR=" .. "LR\"(" .. (os.realpath("data")) .. ")\"" }
	defines { "AM_DATA_DIR=L\"data\"" }
	
	filter { "not options:standalone" }
		include_vendors { "shv" }
	filter{}
	
	include_vendors {
		"imgui",
		"imguizmo",
		"implot",
		"tinyxml2",
		"minhook",
		"directxtex",
		"directxtk",
		"freetype",
		"zlib",
		"nvtt",
		"magic_enum",
		"cgltf",
		"ufbx",
		"hwbreakpoint",
		"stb_image",
		"bc7enc",
		"libwebp",
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
