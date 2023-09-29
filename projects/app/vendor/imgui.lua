vendor_files "imgui-docking/" {
	"imconfig.h",
	"imgui.cpp",
	"imgui.h",
	"imgui_demo.cpp",
	"imgui_draw.cpp",
	"imgui_internal.h",
	-- "imgui_stdlib.cpp",
	-- "imgui_stdlib.h",
	"imgui_tables.cpp",
	"imgui_widgets.cpp",
	"imstb_rectpack.h",
	"imstb_textedit.h",
	"imstb_truetype.h",
	"imgui_impl_win32.h",
	"imgui_impl_win32.cpp",

	"misc/freetype/imgui_freetype.h",
	"misc/freetype/imgui_freetype.cpp",
}

defines { "IMGUI_DISABLE_OBSOLETE_FUNCTIONS", "IMGUI_ENABLE_FREETYPE", "IMGUI_DEFINE_MATH_OPERATORS" }
