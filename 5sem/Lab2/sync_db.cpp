#include <windows.h>
#include <iostream>
#include <vector>


template<typename T>
class SyncDatabase {
private:
    HANDLE hFile;
    const char* filename;

public:
    SyncDatabase(const char* dbFile) : filename(dbFile) {
        hFile = CreateFileA(filename, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile == INVALID_HANDLE_VALUE) {
            std::cerr << "Failed to open file\n";
        }
    }

    ~SyncDatabase() {
        CloseHandle(hFile);
    }

    void addRecord(const T& record) {
        SetFilePointer(hFile, 0, nullptr, FILE_END);
        DWORD bytesWritten;
        WriteFile(hFile, &record, sizeof(T), &bytesWritten, nullptr);
    }

    void getRecord(size_t index) {
        SetFilePointer(hFile, index * sizeof(T), nullptr, FILE_BEGIN);
        DWORD bytesRead;
		T record = {};
        ReadFile(hFile, &record, sizeof(T), &bytesRead, nullptr);
    }

    void deleteRecord(size_t index) {
        T emptyRecord = {};
        SetFilePointer(hFile, index * sizeof(T), nullptr, FILE_BEGIN);
        DWORD bytesWritten;
        WriteFile(hFile, &emptyRecord, sizeof(T), &bytesWritten, nullptr);
    }
};
