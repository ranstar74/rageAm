local dir = "libwebp-0.4.1/"

files { dir .. "include/**.h" }
includedirs { dir .. "include/"}

libdirs { dir .. "lib/" }
links { "libwebp.lib" }
