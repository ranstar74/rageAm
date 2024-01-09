local dir = "lunasvg/"

files { dir .. "**.h", dir .. "**.c", dir .. "**.cpp" }
includedirs { dir }
includedirs { dir .. "plutovg/" }
