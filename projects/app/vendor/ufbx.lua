local dir = "ufbx-0.6.1/"

files { dir .. "*.h", dir .. "*.c", dir .. "ufbx.natvis" }
includedirs { dir }

filter "files:**.natvis"
	buildaction "Natvis"
filter{}

defines { "UFBX_REAL_IS_FLOAT" }