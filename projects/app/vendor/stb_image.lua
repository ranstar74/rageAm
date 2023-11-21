local dir = "stb_image/"

files { dir .. "*.h", dir .. "*.cpp" }
includedirs { dir }

defines { "STBIW_WINDOWS_UTF8" }
