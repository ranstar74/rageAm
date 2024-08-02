#pragma once

#include "am/types.h"

// Services
#include "filedevice.h"

#include <grpcpp/grpcpp.h>

namespace rageam::remote
{
	static constexpr ConstString SERVER_ADDRESS = "localhost:50051";

	/**
	 * \brief Server for communications with rageAm Studio.
	 */
	class RemoteServer : Singleton<RemoteServer>
	{
		amUPtr<grpc::Server> m_Server;
		bool				 m_Initialized = false;
		// Services
		amUPtr<FileDeviceImpl> m_FileDevice;

		void RegisterServices(grpc::ServerBuilder& builder)
		{
			m_FileDevice = std::make_unique<FileDeviceImpl>();
			builder.RegisterService(m_FileDevice.get());
		}

	public:
		RemoteServer() { }
		~RemoteServer() override
		{
			m_Server->Shutdown();
		}

		void Update()
		{
			// Server must be initialized from main thread
			if (!m_Initialized)
			{
				grpc::ServerBuilder builder;
				builder.AddListeningPort(SERVER_ADDRESS, grpc::InsecureServerCredentials());
				RegisterServices(builder);
				m_Server = builder.BuildAndStart();
				AM_TRACEF("[GRPC] Listening on %s", SERVER_ADDRESS);
				m_Initialized = true;
			}
		}
	};
}
