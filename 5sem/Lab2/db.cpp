#include <windows.h>
#include <iostream>
#include <vector>
#include <cstring>

struct Metadata {
    size_t record_count;   // Количество реальных записей (не удалённых)
    size_t total_count;    // Общее количество записей, включая удалённые
    size_t free_list_count;   // Количество свободных индексов
    size_t free_list_size; // Размер списка свободных элементов
};

size_t initialFreeListSize = 5;
size_t minInitialSize = 5;

template<typename T>
class Database {
private:
    HANDLE hFile;
    HANDLE hMapping;
    void* pBase;
    Metadata* pMetadata;

    const char* filename;

    struct Entity {
        size_t id;
        bool is_deleted = false;
        T data;
    };

    Entity* pRecords;

    // Отображение файла в память
    bool mapFileToMemory(size_t size = 1024) {
        hFile = CreateFileA(filename, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, nullptr);
        if (hFile == INVALID_HANDLE_VALUE) {
            std::cerr << "Failed to open file\n";
            return false;
        }

        DWORD fileSize = GetFileSize(hFile, nullptr);
        if (fileSize == INVALID_FILE_SIZE) {
            std::cerr << "Failed to get file size\n";
            return false;
        }
        else if (fileSize == 0)  {
            DWORD initialSize = size * size; // 1 МБ
            LARGE_INTEGER liFileSize;
            liFileSize.QuadPart = initialSize;

            if (!SetFilePointerEx(hFile, liFileSize, NULL, FILE_BEGIN) || !SetEndOfFile(hFile)) {
                printf("Не удалось установить размер файла. Ошибка: %d\n", GetLastError());
                CloseHandle(hFile);
                return false;
            }
        }

        hMapping = CreateFileMapping(hFile, nullptr, PAGE_READWRITE, 0, 0, nullptr);
        if (!hMapping) {
            std::cerr << "Failed to create file mapping\n";
            CloseHandle(hFile);
            return false;
        }

        pBase = MapViewOfFile(hMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
        if (!pBase) {
            std::cerr << "Failed to map view of file\n";
            CloseHandle(hMapping);
            CloseHandle(hFile);
            return false;
        }

        pMetadata = reinterpret_cast<Metadata*>(pBase);
        
        return true;
    }

    // Сохранить изменения и освободить ресурсы
    void unmapFile() {
        UnmapViewOfFile(pBase);
        CloseHandle(hMapping);
        CloseHandle(hFile);
    }

    size_t* getFreeList() {
        return reinterpret_cast<size_t*>(reinterpret_cast<char*>(pBase) + sizeof(Metadata));
    }

    // Расширение базы данных
    std::string expandDatabase() {
        size_t oldSize = sizeof(Metadata) + pMetadata->free_list_size * sizeof(size_t)
        + pMetadata->total_count * sizeof(Entity);
        // std::cout << "\nRecord count: " << pMetadata->record_count << "\n";
        // std::cout << "Total count: " << pMetadata->total_count << "\n";
        
        pMetadata->total_count *= 3;
        if(float(pMetadata->free_list_count)/pMetadata->free_list_size > 0.7)
            pMetadata->free_list_size *= 2;
        size_t newSize = sizeof(Metadata) + pMetadata->free_list_size * sizeof(size_t) 
        + pMetadata->total_count * sizeof(Entity);

        std::string msg = "";
        // msg = "Expanding database...\nOld size: " + std::to_string(oldSize) + " bytes, New size: " + std::to_string(newSize) + " bytes.";

        // std::cout << msg + "\n\n";

        SetFilePointer(hFile, newSize, nullptr, FILE_BEGIN);
        SetEndOfFile(hFile);

        unmapFile();
        mapFileToMemory(newSize);

        return msg;
    }

    std::string compactDatabase() {
        if(pMetadata->total_count < minInitialSize) return "";
        size_t oldSize = sizeof(Metadata) + pMetadata->free_list_size * sizeof(size_t) + 
        pMetadata->total_count * sizeof(Entity);
        size_t writeIndex = 0;
        if(pMetadata->record_count > 0) {
            for (size_t i = 0; i < pMetadata->total_count; ++i) {
                if (!pRecords[i].is_deleted && pRecords[i].id > 0) {
                    if (writeIndex != i) {
                        pRecords[writeIndex] = pRecords[i];
                        pRecords[i].is_deleted = true;
                        pRecords[i].id = 0;
                    }
                    writeIndex++;
                }
            }
        }
        // std::cout << "\nRecord count: " << pMetadata->record_count << "\n";
        // std::cout << "Total count: " << pMetadata->total_count << "\n";

        pMetadata->record_count = writeIndex;
        if(float(pMetadata->free_list_count)/pMetadata->free_list_size < 0.5 && 
                                            pMetadata->free_list_size > initialFreeListSize)
            pMetadata->free_list_size /= 2;
        pMetadata->free_list_count = 0;
        pMetadata->total_count = writeIndex > 0 ? writeIndex * 2 : pMetadata->total_count / 3;
        size_t newSize = sizeof(Metadata) + pMetadata->free_list_size * sizeof(size_t) + 
        pMetadata->total_count * sizeof(Entity);

        std::string msg = "";
        // msg = "Compacting database...\nOld size: " + std::to_string(oldSize) + " bytes, New size: " + std::to_string(newSize) + " bytes.";

        // std::cout << msg + "\n\n";

        SetFilePointer(hFile, newSize, nullptr, FILE_BEGIN);
        SetEndOfFile(hFile);

        unmapFile();
        mapFileToMemory(newSize);
        return msg;
    }

    // Проверка необходимости расширения или сжатия базы данных
    std::string checkAndResize() {
        std::string msg = "";
        float loadFactor = static_cast<float>(pMetadata->record_count) / pMetadata->total_count;
        float deletedFactor = static_cast<float>(pMetadata->free_list_count) / pMetadata->total_count;

        if (loadFactor > 0.7)
            msg = expandDatabase();
        else if (deletedFactor > 0.3)
            msg = compactDatabase();
        return msg;
    }

    size_t getFreeIndex() {
        size_t* free_list = getFreeList();
        size_t index = free_list[0];
        size_t pos = 0;
        size_t count = pMetadata->free_list_count;
        for(int i = 1; i < count; ++i) {
            if(index > free_list[i]) {
                index = free_list[i];
                pos = i;
            }
        }
        free_list[pos] = free_list[count - 1];
        --pMetadata->free_list_count;
        return index;
    }

    size_t countId(size_t index) {
        if(pMetadata->record_count > 0) {
            if(index != 0)
                return pRecords[index - 1].id + 1;
            else {
                long long count = 1;
                Entity* ent = (pRecords + 1);
                while(count <= pMetadata->total_count && (ent->is_deleted || ent->id == 0)) {
                    ++count;
                    ent = (pRecords + count);
                }
                if(ent->id == 0) return 1;
                return ent->id - count;         
            }
        }
        return index + 1;
    }

public:
    Database(const char* dbFile, size_t initialSize = minInitialSize) : hFile(nullptr), hMapping(nullptr), pBase(nullptr), 
    filename(dbFile) {
        if (mapFileToMemory()) {
            if (pMetadata->total_count == 0) {
                pMetadata->record_count = 0;
                pMetadata->total_count = initialSize;
                pMetadata->free_list_count = 0;
                pMetadata->free_list_size = initialFreeListSize;
            }
            pRecords = reinterpret_cast<Entity*>(reinterpret_cast<char*>(pBase) + sizeof(Metadata) +                    pMetadata->free_list_size * sizeof(size_t));
        }
    }

    ~Database() {
        unmapFile();
    }

    // Добавление записи
    std::string addRecord(const T& record) {
        std::string msg = checkAndResize(); 
        if (pMetadata->free_list_count > 0) {
            size_t index = getFreeIndex();
            pRecords[index].data = record;
            pRecords[index].is_deleted = false;
            pRecords[index].id = countId(index);
        } else {
            size_t index = pMetadata->record_count;
            pRecords[index].data = record;
            pRecords[index].is_deleted = false;
            pRecords[index].id = index == 0 ? 1 : pRecords[index - 1].id + 1;
        }
        ++pMetadata->record_count;
        return msg;
    }

    // Чтение записи по индексу
    T getRecord(size_t id) {
        if (id < 1)
            throw std::out_of_range("Index out of range");

        for(int i = 0; i < pMetadata->total_count; ++i) {
            if(!pRecords[i].is_deleted && pRecords[i].id == id)
                return pRecords[i].data;
        }

        throw std::runtime_error("Entity doesn't exist");
    }

    // Удаление записи
    std::string deleteRecord(size_t id) {
        if (id < 1) {
            throw std::out_of_range("Index out of range");
        }
        
        Entity* rec = nullptr;
        int i = 0;
        while(i < pMetadata->total_count) {
            if(!pRecords[i].is_deleted && pRecords[i].id == id) {
                rec = (pRecords + i);
                break;
            } 
            ++i;  
        }

        if(!rec) {
            std::cout << "Entity doesn't exist!\n";
            return "";
        }

        rec->is_deleted = true;
        size_t* free_list = getFreeList();
        free_list[pMetadata->free_list_count] = i;
        ++pMetadata->free_list_count;
        --pMetadata->record_count;
        // std::cout << "Entity --> { ID: " << id << rec->data.toString() << "} <-- deleted.\n";
        return checkAndResize();
    }

    // Получить количество не удалённых записей
    size_t countRecords() {
        return pMetadata->record_count;
    }

    // Получить кол-во удалённых записей
    size_t countDeleted() {
        return pMetadata->free_list_count;
    }
    
    // Получить общий размер отображения
    size_t countTotal() {
        return pMetadata->total_count;
    }

    // Получить все неудалённые записи
    std::string getAllExisting() {
        std::string res;
        for (size_t i = 0; i < pMetadata->total_count; ++i) {
            if (!pRecords[i].is_deleted && pRecords[i].id > 0)
                res += "ID: " + std::to_string(pRecords[i].id) + pRecords[i].data.toString() + "\n";
        }
        return res;
    }

    // Получить все записи
    std::string getAll() {
        std::string res;
        std::string buf = "\n";
        for (size_t i = 0; i < pMetadata->total_count; ++i) {
            if (pRecords[i].id > 0) {
                res += "ID: " + std::to_string(pRecords[i].id)  + pRecords[i].data.toString();
                if(pRecords[i].is_deleted) buf = " <--- DELETED\n";
                res += buf;
                buf = "\n";
            }
        }
        return res;
    }
};


// struct Record {
//     char name[32];
//     int age;

//     std::string toString() const {
//         return " --- Name: " + std::string(name) + " --- Age: " + std::to_string(age);
//     }
// };


// int main() {
//     Database<Record> db("test.db");

//     Record r1 = {"John Doe", 25};
//     Record r2 = {"Jane Smith", 30};
//     Record r3 = {"Bob Smith", 40};
//     Record r4 = {"Clark Kent", 35};
 
//     db.addRecord(r1);
//     db.addRecord(r2);
//     db.addRecord(r3);
//     db.addRecord(r4);

//     std::cout << "All records after insertion:\n";
//     std::cout << db.getAllExisting() << "\n";

//     db.deleteRecord(1);
//     db.deleteRecord(2);

//     std::cout << "All records after delete:\n";
//     std::cout << db.getAllExisting() << "\n";


//     return 0;
// }
