#include <iostream>
#include <vector>
#include <string>
#include <windows.h>
#include <cwctype>
#include <locale>
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

class FAT32FileSearcher
{
private:
    void FindFilesInDirectory(const std::wstring& directory, const std::wstring& shortName, std::vector<std::tuple<std::wstring, std::wstring, std::wstring>>& foundFiles)
    {
        WIN32_FIND_DATAW findFileData;
        std::wstring searchPath = directory + L"*";

        HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findFileData);

        if(hFind == INVALID_HANDLE_VALUE) return;

        do
        {
            std::wstring fileName(findFileData.cFileName);
            std::wstring alternateFileName(findFileData.cAlternateFileName);

            if(fileName == L"." || fileName == L"..") continue;

            wchar_t combinedPath[MAX_PATH];
            PathCombineW(combinedPath, directory.c_str(), fileName.c_str());
            std::wstring fullPath(combinedPath);

            if(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                FindFilesInDirectory(fullPath + L"\\", shortName, foundFiles);
            } else
            {
                wchar_t shortFilePath[MAX_PATH];
                DWORD shortPathLength = GetShortPathNameW(fullPath.c_str(), shortFilePath, MAX_PATH);
                std::wstring fullShortPath;
                std::wstring shortFileName;

                if(shortPathLength > 0 && shortPathLength < MAX_PATH)
                {
                    fullShortPath = std::wstring(shortFilePath);

                    size_t lastBackslash = fullShortPath.find_last_of(L'\\');

                    if(lastBackslash != std::wstring::npos)
                    {
                        shortFileName = fullShortPath.substr(lastBackslash + 1);
                    } else
                    {
                        shortFileName = fullShortPath;
                    }

                    std::wstring lowerShortFileName = ToLowerWString(shortFileName);
                    std::wstring lowerSearchName = ToLowerWString(shortName);

                    if(lowerShortFileName.find(lowerSearchName) != std::wstring::npos)
                    {
                        foundFiles.push_back({ fullShortPath, fileName, shortFileName });
                    }
                }
            }

        } while(FindNextFileW(hFind, &findFileData) != 0);

        FindClose(hFind);
    }

    std::wstring ToLowerWString(const std::wstring& input)
    {
        std::wstring result;
        std::locale loc("");

        for(wchar_t c : input) result += std::tolower(c, loc);

        return result;
    }

    bool IsFAT32Drive(const std::wstring& drive)
    {
        wchar_t fileSystemName[MAX_PATH] = { 0 };

        if(GetVolumeInformationW(drive.c_str(), NULL, 0, NULL, NULL, NULL, fileSystemName, MAX_PATH))
        {
            return _wcsicmp(fileSystemName, L"FAT32") == 0;
        }

        return false;
    }

public:
    std::vector<std::wstring> GetFAT32Drives()
    {
        std::vector<std::wstring> fat32Drives;
        wchar_t drivesBuffer[256] = { 0 };

        DWORD result = GetLogicalDriveStringsW(256, drivesBuffer);

        if(result > 0 && result < 256)
        {
            wchar_t* drive = drivesBuffer;

            while(*drive)
            {
                UINT driveType = GetDriveTypeW(drive);

                if((driveType == DRIVE_REMOVABLE || driveType == DRIVE_FIXED) && IsFAT32Drive(drive))
                {
                    fat32Drives.push_back(std::wstring(drive));
                }

                drive += wcslen(drive) + 1;
            }
        }

        return fat32Drives;
    }

    void FindFiles(const std::wstring& shortName)
    {
        std::vector<std::tuple<std::wstring, std::wstring, std::wstring>> foundFiles;
        std::vector<std::wstring> drives = GetFAT32Drives();

        if(drives.empty())
        {
            std::wcout << L"No FAT32 drives found." << std::endl;
            return;
        }

        for(const auto& drive : drives)
        {
            std::wcout << L"\nScanning FAT32 drive: " << drive << std::endl;

            FindFilesInDirectory(drive, shortName, foundFiles);

            if(!foundFiles.empty())
            {
                std::wstring lastDir;

                for(const auto& [fullShortPath, fileName, shortFileName] : foundFiles)
                {
                    size_t lastSlash = fullShortPath.find_last_of(L"\\");
                    std::wstring currentDirShort = (lastSlash != std::wstring::npos)
                        ? fullShortPath.substr(0, lastSlash)
                        : fullShortPath;

                    if(currentDirShort != lastDir)
                    {
                        std::wcout << L"\nFound list of files in path: \"" << currentDirShort << L"\"" << std::endl;
                        lastDir = currentDirShort;
                    }

                    std::wcout << L"Found file: \"" << shortFileName << L"\" (Long name: \"" << fileName << L"\") in Path: \"" << fullShortPath << L"\"" << std::endl;
                }

                foundFiles.clear();
            } else
            {
                std::wcout << L"No matching files found on drive " << drive << std::endl;
            }
        }
    }
};

int main()
{
    SetConsoleOutputCP(CP_UTF8);
    std::locale::global(std::locale(""));

    FAT32FileSearcher searcher;

    std::wcout << L"Enter the short file name: ";
    std::wstring shortName;
    std::getline(std::wcin, shortName);

    searcher.FindFiles(shortName);

    return 0;
}