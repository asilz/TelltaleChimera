#include <cerrno>
#include <cstdio>
#include <cstring>

int FileBrowsePath(char *path, size_t maxLength)
{
    FILE *f = popen("zenity --file-selection", "r");
    if (f == nullptr)
    {
        return errno;
    }
    fgets(path, maxLength, f);
    path[strnlen(path, maxLength) - 1] = '\0';
    return pclose(f);
}

int FileSavePath(char *path, size_t maxLength)
{
    FILE *f = popen("zenity --save --file-selection", "r");
    if (f == nullptr)
    {
        return errno;
    }
    fgets(path, maxLength, f);
    path[strnlen(path, maxLength) - 1] = '\0';
    return pclose(f);
}