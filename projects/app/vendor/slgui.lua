vendor_files "slgui/" {
	"imconfig.h",
	"imgui.cpp",
	"imgui.h",	
	"imgui_gradient.h",
	"imgui_gradient.cpp",
	"imgui_demo.cpp",
	"imgui_draw.cpp",
	"imgui_internal.h",
	"imgui_tables.cpp",
	"imgui_widgets.cpp",
	"imstb_rectpack.h",
	"imstb_textedit.h",
	"imstb_truetype.h",
	"backends/imgui_impl_win32.h",
	"backends/imgui_impl_win32.cpp",

	"misc/freetype/imgui_freetype.h",
	"misc/freetype/imgui_freetype.cpp",
}

defines
{
	"IMGUI_DISABLE_OBSOLETE_FUNCTIONS",
	"IMGUI_ENABLE_FREETYPE",
	"IMGUI_USE_STB_SPRINTF",
	"IMGUI_DISABLE_STB_SPRINTF_IMPLEMENTATION",
	"IMGUI_DEFINE_MATH_OPERATORS"
}
