files { "nvtt/include/**.h", "nvtt/include/**.cpp" }
includedirs { "nvtt/include" }

libdirs { "nvtt/lib/" }

links { "nvtt30201" }

-- It's not really good that we use out of scope path here
postbuildcommands { "{COPY} " .. (os.realpath("nvtt/nvtt30201.dll")) .. " " .. (os.realpath("../../../bin/%{cfg.buildcfg}")) }
