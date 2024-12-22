#include <Windows.h>
#include <cstdio>

int FileBrowsePath(char *buf, size_t maxLength)
{
    OPENFILENAME open = {0};
    open.lStructSize = sizeof(OPENFILENAME);
    open.lpstrFilter = ".skl, .anm or .d3dmesh\0*.skl;*.anm;*.d3dmesh\0";
    open.lpstrFile = buf;
    open.lpstrFile[0] = '\0';
    open.nMaxFile = maxLength;
    open.nFilterIndex = 1;
    open.lpstrTitle = "Select a .skl, .anm or .d3dmesh file\0";
    open.nMaxFileTitle = strlen(open.lpstrTitle);
    open.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;
    return !GetOpenFileName(&open);
}
int FileSavePath(char *buf, size_t maxLength)
{
    OPENFILENAME open = {0};
    open.lStructSize = sizeof(OPENFILENAME);
    open.lpstrFilter = ".glb\0*.glb\0";
    open.lpstrFile = buf;
    open.lpstrFile[0] = '\0';
    open.nMaxFile = maxLength;
    open.nFilterIndex = 1;
    open.lpstrTitle = "Save a .glb file\0";
    open.nMaxFileTitle = strlen(open.lpstrTitle);
    open.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;
    return !GetSaveFileName(&open);
}