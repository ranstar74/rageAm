#pragma once

#include "am/system/dispatcher.h"
#include "am/types.h"
#include "am/file/device.h"
#include "am/crypto/cipher.h"

#include <protoc/filedevice.grpc.pb.h>

namespace rageam::remote
{
	class FileDeviceImpl : public FileDevice::Service
	{
		struct ResponseEntry
		{
			char Path[MAX_PATH];
			u32  Version	: 8; // For resources
			u32  Encryption : 2; // rageam::file::FileEncryption
			u64  Size;
			u64  Packed;
			u64  Modified;
			u64  Type		  : 2;  // rageam::file::FileType
			u64  VirtualSize  : 28; // 256MB~
			u64  PhysicalSize : 28;
		};

		static ResponseEntry CreateResponseEntry(const file::FileSearchData& search)
		{
			ImmutableWString path = search.Path.GetCStr();

			ResponseEntry responseEntry = {};
			String::Copy(responseEntry.Path, MAX_PATH, PATH_TO_UTF8(search.Path));

			// General Parameters
			responseEntry.Type = search.Type;
			if (search.Entry)
			{
				responseEntry.Size = search.Packfile->GetEntrySize(*search.Entry);
				responseEntry.Packed = search.Packfile->GetEntryCompressedSize(*search.Entry);
				responseEntry.Modified = search.Packfile->GetFileTime("" /* Unused parameter... Root packfile time is always returned */);
			}
			else
			{
				if (search.Type != file::FileEntry_Directory) // Directories don't have size...
					responseEntry.Size = GetFileSize64(search.Path);
				responseEntry.Packed = 0; // Although there are compressed Zip/Rar files in OS file system, we use it really only for packfiles
				responseEntry.Modified = GetFileModifyTime(search.Path);
			}

			// Encryption - Resources (except ysc) / directories are never encrypted
			u32 encryption = 0;
			if (path.EndsWith(L".ysc")) // Scripts are always encrypted, however this is not defined in packfile explicitely
			{
				encryption = file::FileEncryption_TFIT;
			}
			else if (search.Type == file::FileEntry_Packfile)
			{
				rage::fiPackHeader header;
				// Packfile encryption is not specified in file entry
				if (search.Entry)
				{
					u64 offset;
					fiHandle_t handle = search.Packfile->OpenBulkEntry(*search.Entry, offset);
					if (search.Packfile->ReadBulk(handle, offset, &header, 16) == 16)
						encryption = header.Encryption;
					search.Packfile->CloseBulk(handle);
				}
				else
				{
					// For OS FS we have to read rpf header from file
					if (GetPackfileHeader(search.Path, header))
						encryption = header.Encryption;
				}
			}
			else if (search.Entry && search.Entry->IsFile())
			{
				encryption = search.Entry->File.Encryption;
			}
			switch (encryption)
			{
			case CIPHER_KEY_ID_TFIT:
				responseEntry.Encryption = file::FileEncryption_TFIT;
				break;
			case CIPHER_KEY_ID_AES:
				responseEntry.Encryption = file::FileEncryption_AES;
				break;
			case CIPHER_KEY_ID_NONE:
			case CIPHER_KEY_ID_OPEN:
				responseEntry.Encryption = file::FileEncryption_None;
				break;
			}
			
			// Resource information
			if (search.Type == file::FileEntry_Resource)
			{
				rage::datResourceInfo info;
				u32 version;
				if (search.Packfile)
					version = search.Packfile->GetResourceInfo(*search.Entry, info);
				else
					version = file::GetResourceInfo(path, info);

				if (version > 0)
				{
					responseEntry.Version = version;
					responseEntry.VirtualSize = info.ComputeVirtualSize();
					responseEntry.PhysicalSize = info.ComputePhysicalSize();
				}
			}
			return responseEntry;
		}

	public:
		grpc::Status ScanDirectory(grpc::ServerContext* context, const FileScanRequest* request, FileScanResponse* response) override
		{
			auto task = Dispatcher::UpdateThread().RunAsync([&]
			{
				file::FileDevice::GetInstance()->ScanDirectory(PATH_TO_WIDE(request->path().c_str()));
			});
			task.get();
			return grpc::Status::OK;
		}

		// Finds all files in specified directory or packfile, supports pattern matching, i.e. '*.ydr'
		grpc::Status Search(grpc::ServerContext* context, const FileSearchRequest* request, grpc::ServerWriter<FileSearchResponse>* writer) override
		{
			// TODO: Use shared memory!

			// Find optimal chunk size for gRPC
			static constexpr u32 BATCH_SIZE = (u32)(16.0 * 1024.0 * 1024 / double(sizeof(ResponseEntry)));

			List<ResponseEntry> batchEntries;
			batchEntries.Reserve(BATCH_SIZE, true);

			FileSearchResponse batchResponse;
			auto sendBatch = [&]
			{
				batchResponse.set_filedata((char*) batchEntries.GetItems(), batchEntries.GetSize() * sizeof ResponseEntry);
				writer->Write(batchResponse);
				batchEntries.Clear();
			};

			file::WPath searchW = PATH_TO_WIDE(request->path().c_str());
			file::WPath patternW = PATH_TO_WIDE(request->pattern().c_str());
			auto task = Dispatcher::UpdateThread().RunAsync([&]
			{
				file::FileDevice::GetInstance()->Search(searchW, patternW, [&](const file::FileSearchData& search)
				{
					ResponseEntry responseEntry = CreateResponseEntry(search);
					batchEntries.Add(responseEntry);
					if (batchEntries.GetSize() == BATCH_SIZE)
						sendBatch();
				}, request->recurse(), request->includeflags());

				// Write remaining
				if (batchEntries.Any())
					sendBatch();
			});
			task.get();

			return grpc::Status::OK;
		}

		grpc::Status IsFileExists(grpc::ServerContext* context, const FileExistsRequest* request, FileExistResponse* response) override
		{
			file::WPath path = PATH_TO_WIDE(request->path().c_str());
			auto task = Dispatcher::UpdateThread().RunAsync([&]
			{
				return file::FileDevice::GetInstance()->IsFileExists(path);
			});
			response->set_value(task.get());
			return grpc::Status::OK;
		}
	};
}
