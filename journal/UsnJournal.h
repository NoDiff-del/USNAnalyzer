#pragma once
#include <windows.h>
#include <winioctl.h>
#include <vector>
#include <string>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <thread>
#include <mutex>
#include <algorithm>
#include <Shlwapi.h>

#pragma comment(lib, "Shlwapi.lib")

namespace USNReader {

    std::wstring GetFilePathFromFileId(HANDLE volumeHandle, ULONGLONG fileId);

    void ReadUSNJournal(const std::wstring& volume);

}