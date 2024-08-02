namespace Rageam;

public static class ListExtensions
{
    /// <summary>
    /// Groups all directories first, order of other entries remain unchanged, just like in Windows explorer
    /// </summary>
    public static void GroupDirectoriesFirst<T>(this List<T> list, Predicate<T> isDirectory)
    {
        list.Sort((x, y) =>
        {
            bool xDir = isDirectory(x);
            bool yDir = isDirectory(y);
            if (xDir && yDir) return 0;
            if (xDir) return -1;
            if (yDir) return 1;
            return 0;
        });
    }
}