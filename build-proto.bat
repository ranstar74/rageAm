%VCPKG_ROOT%\packages\protobuf_x64-windows\tools\protobuf\protoc ^
	--cpp_out=projects\app\src\protoc ^
	--proto_path=projects\app\src\protos ^
	--grpc_out=projects\app\src\protoc ^
	--plugin=protoc-gen-grpc=%VCPKG_ROOT%\packages\grpc_x64-windows\tools\grpc\grpc_cpp_plugin.exe ^
	filedevice.proto
pause