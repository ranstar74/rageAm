using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using BitsKit;
using BitsKit.BitFields;
using Rageam.Remote;

namespace Rageam;

[StructLayout(LayoutKind.Sequential)]
[BitObject(BitOrder.LeastSignificant)]
public unsafe partial struct FileEntry // Must match declaration in rageam::remote::FileDeviceImpl
{
    private fixed byte _path[260];

    [BitField("Version", 8)] [BitField("Encryption", 2)]
    private uint _bitField0;

    public ulong Size;
    public ulong Packed;
    private long _modified;

    [BitField("Type", 2)] [BitField("VirtualSize", 28)] [BitField("PhysicalSize", 28)]
    private ulong _bitField1;

    public string Path
    {
        get
        {
            fixed (byte* path = _path)
                return Marshal.PtrToStringAnsi(new IntPtr(path));
        }
    }

    public DateTime Modified => DateTime.FromFileTime(_modified);
    public override string ToString() => Path;
}

public static class FileDevice
{
    private static readonly Remote.FileDevice.FileDeviceClient Device = new(RemoteClient.Channel);

    /// <summary>
    /// Pre-loads all packfiles (.RPF) recursively in specified directory, recommended to call before Search
    /// </summary>
    public static async Task ScanDirectory(string path)
    {
        FileScanRequest request = new()
        {
            Path = path
        };
        await Device.ScanDirectoryAsync(request);
    }

    /// <summary>
    /// Gets all file entries in specified directory, or packfile (.RPF)
    /// </summary>
    /// <param name="path">Search directory path</param>
    /// <param name="pattern">Pattern in glob format ('*' / '?') or regular string. If glob pattern is not recognized, default 'contains' operation is used instead</param>
    /// <param name="recurse">Whether it is needed to scan all nested subdirectories or packfiles</param>
    /// <param name="includeFlags">What kind of entries to include in search</param>
    public static async IAsyncEnumerable<FileEntry> Search(string path, string pattern, bool recurse, FileSearchIncludeFlags includeFlags, [EnumeratorCancellation] CancellationToken cancellationToken)
    {
        FileSearchRequest request = new()
        {
            Path = path,
            Pattern = pattern,
            Recurse = recurse,
            IncludeFlags = (int)includeFlags
        };
        var response = Device.Search(request, cancellationToken: cancellationToken);
        while (await response.ResponseStream.MoveNext(cancellationToken))
        {
            FileSearchResponse fileSearchResponse = response.ResponseStream.Current;
            byte[] fileData = fileSearchResponse.FileData.ToByteArray();
            FileEntry[] entries = MemoryMarshal.Cast<byte, FileEntry>(fileData.AsSpan()).ToArray();
            foreach (FileEntry entry in entries)
                yield return entry;
        }
    }

    /// <summary>
    /// Gets whether file, directory, packfile or entry in a packfile at given path exists
    /// </summary>
    public static async Task<bool> IsFileExists(string path)
    {
        FileExistsRequest request = new()
        {
            Path = path
        };
        FileExistResponse response = await Device.IsFileExistsAsync(request);
        return response.Value;
    }
}