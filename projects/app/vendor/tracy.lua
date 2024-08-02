files { "tracy-0.11.0/**.h", "tracy-0.11.0/**.hpp" }

--libdirs { "tracy-0.11.0/bin" }
libdirs { "tracy-0.11.0/lib" }
--links { "TracyProfiler.dll" }
links { "TracyProfiler.lib" }

defines { "TRACY_ENABLE", "TRACY_IMPORTS" }

includedirs { "tracy-0.11.0/include/" }
