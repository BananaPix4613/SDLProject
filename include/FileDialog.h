#pragma once

#include <string>
#include <Windows.h>
#include <commdlg.h>
#include <ShlObj.h>

class FileDialog
{
public:
    // Show open file dialog and return the selected file path
    static std::string openFile(const char* filter = "Binary Files (*.bin)\0*.bin\0All Files\0*.*\0", HWND owner = NULL)
    {
        OPENFILENAMEA ofn;
        char fileName[MAX_PATH] = "";
        ZeroMemory(&ofn, sizeof(ofn));

        ofn.lStructSize = sizeof(OPENFILENAMEA);
        ofn.hwndOwner = owner;
        ofn.lpstrFilter = filter;
        ofn.lpstrFile = fileName;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
        ofn.lpstrDefExt = "bin";

        std::string fileNameStr;
        if (GetOpenFileNameA(&ofn))
        {
            fileNameStr = fileName;
        }

        return fileNameStr;
    }

    // Show save file dialog and return the selected file path
    static std::string saveFile(const char* filter = "Binary Files (*.bin)\0*.bin\0All Files\0*.*\0", HWND owner = NULL)
    {
        OPENFILENAMEA ofn;
        char fileName[MAX_PATH] = "";
        ZeroMemory(&ofn, sizeof(ofn));

        ofn.lStructSize = sizeof(OPENFILENAMEA);
        ofn.hwndOwner = owner;
        ofn.lpstrFilter = filter;
        ofn.lpstrFile = fileName;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
        ofn.lpstrDefExt = "bin";

        std::string fileNameStr;
        if (GetSaveFileNameA(&ofn))
        {
            fileNameStr = fileName;
        }

        return fileNameStr;
    }

    // Show folder browser dialog and return the selected folder path
    static std::string selectFolder(const char* title = "Select Folder", HWND owner = NULL)
    {
        std::string folderPath;

        // Initialize COM for this thread
        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        if (SUCCEEDED(hr))
        {
            IFileDialog* pfd;
            hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));

            if (SUCCEEDED(hr))
            {
                DWORD dwOptions;
                hr = pfd->GetOptions(&dwOptions);
                if (SUCCEEDED(hr))
                {
                    // Set folder picker mode
                    hr = pfd->SetOptions(dwOptions | FOS_PICKFOLDERS);
                    if (SUCCEEDED(hr))
                    {
                        // Convert title string to wide string for SetTitle
                        WCHAR wideTitle[256];
                        MultiByteToWideChar(CP_ACP, 0, title, -1, wideTitle, 256);
                        pfd->SetTitle(TEXT(wideTitle));

                        // Show the dialog
                        hr = pfd->Show(owner);
                        if (SUCCEEDED(hr))
                        {
                            IShellItem* psi;
                            hr = pfd->GetResult(&psi);
                            if (SUCCEEDED(hr))
                            {
                                PWSTR pszFilePath;
                                hr = psi->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
                                if (SUCCEEDED(hr))
                                {
                                    // Convert wide string to narrow string
                                    int bufferSize = WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, NULL, 0, NULL, NULL);
                                    if (bufferSize > 0)
                                    {
                                        std::string result(bufferSize - 1, 0);
                                        WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, &result[0], bufferSize, NULL, NULL);
                                        folderPath = result;
                                    }
                                    CoTaskMemFree(pszFilePath);
                                }
                                psi->Release();
                            }
                        }
                    }
                }
                pfd->Release();
            }
            CoUninitialize();
        }

        return folderPath;
    }
};