#include <windows.h>
#include <string>
#include <iostream>

const int BUFFER_COUNT = 5;     // Количество буферов в пуле
const int BUFFER_SIZE = 256;    // Размер одного буфера

// Структура для описания буфера
struct SharedBuffer {
    int id;
    char data[BUFFER_SIZE];
    bool is_used;
};

// Структура для хранения всей разделяемой памяти
struct SharedMemoryData {
    SharedBuffer buffers[BUFFER_COUNT];
};

// Класс для управления разделяемой памятью
class SharedMemory {
private:
    HANDLE hMapFile;
    SharedMemoryData* shared_memory;

public:
    SharedMemory(const std::string& name) {
        // Создание или открытие разделяемой памяти
        hMapFile = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(SharedMemoryData), name.c_str());
        if (hMapFile == NULL) {
            throw std::runtime_error("CreateFileMapping failed");
        }

        // Отображение разделяемой памяти в адресное пространство процесса
        shared_memory = (SharedMemoryData*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedMemoryData));
        if (shared_memory == NULL) {
            CloseHandle(hMapFile);
            throw std::runtime_error("MapViewOfFile failed");
        }

        // Инициализация буферов (если память была создана)
        if (GetLastError() != ERROR_ALREADY_EXISTS) {
            for (int i = 0; i < BUFFER_COUNT; ++i) {
                shared_memory->buffers[i].id = i + 1;
                shared_memory->buffers[i].is_used = false;
            }
        }
    }

    // Возвращает указатель на буфер по индексу
    SharedBuffer* getBuffer(int index) {
        return &shared_memory->buffers[index];
    }

    // Освобождение ресурсов
    ~SharedMemory() {
        UnmapViewOfFile(shared_memory);
        CloseHandle(hMapFile);
    }
};