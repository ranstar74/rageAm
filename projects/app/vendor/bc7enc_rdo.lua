local dir = "bc7enc_rdo/"

files { dir .. "*.h", dir .. "*.c", dir .. "*.cpp"  }
includedirs { dir }

libdirs { dir }
links 
{ 
	"bc7e_avx.obj", 
	"bc7e_avx2.obj", 
	"bc7e_sse2.obj", 
	"bc7e_sse4.obj", 
	"bc7e.obj"
}
