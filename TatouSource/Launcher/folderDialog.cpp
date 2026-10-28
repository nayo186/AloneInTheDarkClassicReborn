///////////////////////////////////////////////////////////////////////////////
// Alone in the Dark: Classic Reborn - launcher
// Based on FITD by yaz0r and AloneInTheDarkReHaunted, released under GPL
//
// Folder picker (see folderDialog.h)
///////////////////////////////////////////////////////////////////////////////

#include "folderDialog.h"

#include <SDL3/SDL.h>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <shobjidl.h>

static std::wstring toWide(const std::string& utf8)
{
    const int length = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if (length <= 0)
        return std::wstring();
    std::wstring wide(length, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wide[0], length);
    wide.resize(length - 1);
    return wide;
}

static std::string toUtf8(const wchar_t* wide)
{
    const int length = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
    if (length <= 0)
        return std::string();
    std::string utf8(length, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, &utf8[0], length, nullptr, nullptr);
    utf8.resize(length - 1);
    return utf8;
}

bool chooseFolderNative(SDL_Window* window, const std::string& startFolder, const std::string& title,
                        std::string& chosen)
{
    // The dialog is a COM object; COM may already be on for this thread (SDL)
    const HRESULT init = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    bool picked = false;
    IFileOpenDialog* dialog = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog))))
    {
        DWORD options = 0;
        dialog->GetOptions(&options);
        dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
        if (!title.empty())
            dialog->SetTitle(toWide(title).c_str());
        if (!startFolder.empty())
        {
            IShellItem* start = nullptr;
            if (SUCCEEDED(SHCreateItemFromParsingName(toWide(startFolder).c_str(), nullptr, IID_PPV_ARGS(&start))))
            {
                dialog->SetFolder(start);
                start->Release();
            }
        }
        HWND owner = window ? (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(window),
            SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr) : nullptr;
        if (SUCCEEDED(dialog->Show(owner)))
        {
            IShellItem* result = nullptr;
            if (SUCCEEDED(dialog->GetResult(&result)))
            {
                PWSTR path = nullptr;
                if (SUCCEEDED(result->GetDisplayName(SIGDN_FILESYSPATH, &path)))
                {
                    chosen = toUtf8(path);
                    picked = !chosen.empty();
                    CoTaskMemFree(path);
                }
                result->Release();
            }
        }
        dialog->Release();
    }
    if (SUCCEEDED(init))
        CoUninitialize();
    return picked;
}

#else

bool chooseFolderNative(SDL_Window*, const std::string&, const std::string&, std::string&)
{
    return false;
}

#endif
