local zlib_dir = "zlib-ng/"

files { zlib_dir .. "*.h", zlib_dir .. "*.c" }
includedirs { zlib_dir }

libdirs { zlib_dir }
links { "zlibstatic-ng.lib" }
