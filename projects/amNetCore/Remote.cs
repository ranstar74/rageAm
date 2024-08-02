using Grpc.Core;
using Grpc.Net.Client;

namespace Rageam;

public static class RemoteClient
{
    public static string Address => "http://localhost:50051";

    public static GrpcChannel Channel { get; } = GrpcChannel.ForAddress(Address, new GrpcChannelOptions()
    {
        MaxReceiveMessageSize = 16 * 1024 * 1024, // 16 Megabytes
        Credentials = ChannelCredentials.Insecure,
        MaxRetryAttempts = int.MaxValue,
    });
}
