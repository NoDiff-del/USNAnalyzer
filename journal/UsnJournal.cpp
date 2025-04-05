#include "UsnJournal.h"

namespace USNReader {

    std::wstring GetFilePathFromFileId(HANDLE volumeHandle, ULONGLONG fileId) {
        FILE_ID_DESCRIPTOR fileIdDesc = {};
        fileIdDesc.dwSize = sizeof(FILE_ID_DESCRIPTOR);
        fileIdDesc.Type = FileIdType;
        fileIdDesc.FileId.QuadPart = fileId;

        HANDLE fileHandle = OpenFileById(volumeHandle, &fileIdDesc, 0,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr, 0);

        if (fileHandle == INVALID_HANDLE_VALUE) {
            return std::to_wstring(fileId);
        }

        std::vector<wchar_t> buffer(1024);
        DWORD result = GetFinalPathNameByHandleW(fileHandle, buffer.data(), static_cast<DWORD>(buffer.size()), FILE_NAME_NORMALIZED);
        CloseHandle(fileHandle);

        if (result == 0) {
            return std::to_wstring(fileId);
        }

        std::wstring fullPath(buffer.data());
        if (fullPath.rfind(L"\\\\?\\", 0) == 0) {
            fullPath = fullPath.substr(4);
        }

        wchar_t pathBuffer[MAX_PATH];
        wcscpy_s(pathBuffer, fullPath.c_str());
        if (PathRemoveFileSpecW(pathBuffer)) {
            std::wstring dir = pathBuffer;
            if (!dir.empty() && dir.back() != L'\\') {
                dir += L'\\';
            }
            return dir;
        }

        return fullPath;
    }

    void ReadUSNJournal(const std::wstring& volume) {
        HANDLE hVolume = CreateFileW((L"\\\\.\\" + volume).c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
        if (hVolume == INVALID_HANDLE_VALUE) {
            return;
        }

        USN_JOURNAL_DATA_V0 journalData = { 0 };
        DWORD bytesReturned = 0;
        if (!DeviceIoControl(hVolume, FSCTL_QUERY_USN_JOURNAL, nullptr, 0, &journalData, sizeof(journalData), &bytesReturned, nullptr)) {
            CloseHandle(hVolume);
            return;
        }

        std::wofstream outputFile("USN_Journal_Log.txt");
        if (!outputFile) {
            CloseHandle(hVolume);
            return;
        }

        USN usn = journalData.FirstUsn;
        std::vector<BYTE> buffer(32 * 1024 * 1024);
        READ_USN_JOURNAL_DATA_V0 readData = { 0 };
        readData.StartUsn = usn;
        readData.ReasonMask = 0xFFFFFFFF;
        readData.ReturnOnlyOnClose = FALSE;
        readData.Timeout = 0;
        readData.BytesToWaitFor = 0;
        readData.UsnJournalID = journalData.UsnJournalID;

        while (DeviceIoControl(hVolume, FSCTL_READ_USN_JOURNAL, &readData, sizeof(readData), buffer.data(), static_cast<DWORD>(buffer.size()), &bytesReturned, nullptr)) {
            if (bytesReturned <= sizeof(USN)) break;

            BYTE* ptr = buffer.data() + sizeof(USN);
            while (ptr < buffer.data() + bytesReturned) {
                USN_RECORD_V2* record = reinterpret_cast<USN_RECORD_V2*>(ptr);
                if (record->RecordLength == 0) break;

                std::wstring fileName(record->FileName, record->FileNameLength / sizeof(WCHAR));
                FILETIME ft;
                SYSTEMTIME st;
                ULONGLONG time = *(ULONGLONG*)&record->TimeStamp;
                ft.dwLowDateTime = (DWORD)(time & 0xFFFFFFFF);
                ft.dwHighDateTime = (DWORD)(time >> 32);

                SYSTEMTIME localTime;
                FileTimeToSystemTime(&ft, &st);
                SystemTimeToTzSpecificLocalTime(nullptr, &st, &localTime);

                std::vector<std::wstring> reasons;
                if (record->Reason & USN_REASON_DATA_OVERWRITE) reasons.push_back(L"Data Overwrite");
                if (record->Reason & USN_REASON_DATA_EXTEND) reasons.push_back(L"Data Extend");
                if (record->Reason & USN_REASON_DATA_TRUNCATION) reasons.push_back(L"Data Truncation");
                if (record->Reason & USN_REASON_NAMED_DATA_OVERWRITE) reasons.push_back(L"Named Data Overwrite");
                if (record->Reason & USN_REASON_NAMED_DATA_EXTEND) reasons.push_back(L"Named Data Extend");
                if (record->Reason & USN_REASON_NAMED_DATA_TRUNCATION) reasons.push_back(L"Named Data Truncation");
                if (record->Reason & USN_REASON_FILE_CREATE) reasons.push_back(L"File Create");
                if (record->Reason & USN_REASON_FILE_DELETE) reasons.push_back(L"File Delete");
                if (record->Reason & USN_REASON_EA_CHANGE) reasons.push_back(L"EA Change");
                if (record->Reason & USN_REASON_SECURITY_CHANGE) reasons.push_back(L"Security Change");
                if (record->Reason & USN_REASON_RENAME_OLD_NAME) reasons.push_back(L"Rename: Old Name");
                if (record->Reason & USN_REASON_RENAME_NEW_NAME) reasons.push_back(L"Rename: New Name");
                if (record->Reason & USN_REASON_INDEXABLE_CHANGE) reasons.push_back(L"Indexable Change");
                if (record->Reason & USN_REASON_BASIC_INFO_CHANGE) reasons.push_back(L"Basic Info Change");
                if (record->Reason & USN_REASON_HARD_LINK_CHANGE) reasons.push_back(L"Hard Link Change");
                if (record->Reason & USN_REASON_COMPRESSION_CHANGE) reasons.push_back(L"Compression Change");
                if (record->Reason & USN_REASON_ENCRYPTION_CHANGE) reasons.push_back(L"Encrytion Change");
                if (record->Reason & USN_REASON_OBJECT_ID_CHANGE) reasons.push_back(L"Object ID Change");
                if (record->Reason & USN_REASON_REPARSE_POINT_CHANGE) reasons.push_back(L"Reparser Point Change");
                if (record->Reason & USN_REASON_STREAM_CHANGE) reasons.push_back(L"Stream Change");
                if (record->Reason & USN_REASON_TRANSACTED_CHANGE) reasons.push_back(L"Transacted Change");
                if (record->Reason & USN_REASON_INTEGRITY_CHANGE) reasons.push_back(L"Integrity Change");
                if (record->SourceInfo & USN_SOURCE_AUXILIARY_DATA) reasons.push_back(L"Auxiliary Data (Private Data Flow)");
                if (record->SourceInfo & USN_SOURCE_DATA_MANAGEMENT) reasons.push_back(L"Data Management (No Data Change)");
                if (record->SourceInfo & USN_SOURCE_REPLICATION_MANAGEMENT) reasons.push_back(L"Replication Management");
                if (record->SourceInfo & USN_SOURCE_CLIENT_REPLICATION_MANAGEMENT) reasons.push_back(L"Client Replication Management");
                if (record->Reason & USN_REASON_CLOSE) reasons.push_back(L"Close");

                if (!reasons.empty()) {
                    std::wstring reasonsJoined;
                    for (size_t i = 0; i < reasons.size(); ++i) {
                        reasonsJoined += reasons[i];
                        if (i < reasons.size() - 1) {
                            reasonsJoined += L" | ";
                        }
                    }

                    std::wstring dirPath = GetFilePathFromFileId(hVolume, record->FileReferenceNumber);

                    outputFile << L"USN: " << record->Usn << L" * "
                        << L"Name: " << fileName << L" * "
                        << L"Date: " << std::setfill(L'0') << std::setw(4) << localTime.wYear << L"-"
                        << std::setw(2) << localTime.wMonth << L"-" << std::setw(2) << localTime.wDay << L" "
                        << std::setw(2) << localTime.wHour << L":" << std::setw(2) << localTime.wMinute << L":" << std::setw(2) << localTime.wSecond
                        << L" * Reason: " << reasonsJoined << L" * Directory: " << dirPath << std::endl;
                }

                ptr += record->RecordLength;
            }

            readData.StartUsn = *(USN*)buffer.data();
        }

        outputFile.close();
        CloseHandle(hVolume);
    }

}