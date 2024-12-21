#include <cerrno>
#include <cstdio>
#include <cstring>

int GetFilePath(char *path, size_t maxLength)
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