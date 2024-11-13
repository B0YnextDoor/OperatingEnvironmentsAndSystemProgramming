#include <windows.h>
#include <iostream>

// Класс для управления мьютексом
class Mutex {
private:
    HANDLE hMutex;

public:
    Mutex(const std::string& name) {
        hMutex = CreateMutexA(NULL, FALSE, name.c_str());
        if (hMutex == NULL) {
            throw std::runtime_error("CreateMutex failed");
        }
    }

    // Захват мьютекса
    void lock() {
        WaitForSingleObject(hMutex, INFINITE);
    }

    // Освобождение мьютекса
    void unlock() {
        ReleaseMutex(hMutex);
    }

    ~Mutex() {
        CloseHandle(hMutex);
    }
};