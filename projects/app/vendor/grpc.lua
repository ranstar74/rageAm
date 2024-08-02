function getFilename(file)
      return file:match("^.+/(.+)$")
end


function install(path)
	includedirs { path .. "include/" }

	files { path .. "**.h" }

	filter { "configurations:Debug" }
		libdirs {path .. "debug/lib"  }
		
		local fullpath_dlls = os.matchfiles(path .. "debug/lib/*.lib")
		local libs = table.translate(fullpath_dlls, getFilename)
		links(libs)
	filter { "configurations:Release" }
		libdirs { path .. "lib" }
		
		local fullpath_dlls = os.matchfiles(path .. "lib/*.lib")
		local libs = table.translate(fullpath_dlls, getFilename)
		links(libs)
	filter {}
end

local vcpkg = os.getenv("VCPKG_ROOT")
install(vcpkg .. "/packages/grpc_x64-windows-static-md/")
install(vcpkg .. "/packages/protobuf_x64-windows-static-md/")
install(vcpkg .. "/packages/abseil_x64-windows-static-md/")
install(vcpkg .. "/packages/upb_x64-windows-static-md/")
install(vcpkg .. "/packages/c-ares_x64-windows-static-md/")
install(vcpkg .. "/packages/openssl_x64-windows-static-md/")
install(vcpkg .. "/packages/re2_x64-windows-static-md/")
install(vcpkg .. "/packages/utf8-range_x64-windows-static-md/")

links("crypt32.lib") -- Needed for openssl
