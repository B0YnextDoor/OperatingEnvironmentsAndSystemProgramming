#include <sstream>
#include "db.cpp"

// Структура для записи (модель)
struct Record {
    char name[32];
    int age;

    std::string toString() const {
        return " --- Name: " + std::string(name) + " --- Age: " + std::to_string(age);
    }
};

// Глобальные переменные
Database<Record>* db;
std::vector<std::string> records;
HWND hEditName, hEditAge, hListBox;

std::vector<std::string> splitText(const std::string& text) {
    std::vector<std::string> lines;
    std::istringstream stream(text);
    std::string line;
    
    while (std::getline(stream, line)) {
        lines.push_back(line);
    }

    return lines;
}

// Функция для вывода списка записей
void ListRecords(HWND hwnd, bool type = false) {
    // Очищаем список
    SendMessage(hListBox, LB_RESETCONTENT, 0, 0);
    records = splitText(!type ? db->getAllExisting() : db->getAll());
    for (const auto& record : records) {
        SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)record.c_str());
    }
}

// Функция для добавления записи
void AddRecord(HWND hwnd) {
    char name[32];
    char ageStr[10];
    int age;

    // Получаем введённые данные
    GetWindowTextA(hEditName, name, 32);
    GetWindowTextA(hEditAge, ageStr, 10);
    age = atoi(ageStr);

    if (strlen(name) == 0 || age <= 0 || age > 150) {
        MessageBoxA(hwnd, "Please enter valid name and age!", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    // Добавляем запись в вектор
    Record newRecord;
    strcpy_s(newRecord.name, name);
    newRecord.age = age;
    std::string msg = db->addRecord(newRecord);
    if(msg.length() > 0)
        MessageBoxA(hwnd, msg.c_str(), "Info", MB_OK | MB_ICONINFORMATION);

    ListRecords(hwnd);

    SetWindowTextA(hEditName, "");
    SetWindowTextA(hEditAge, "");
}

// Функция для удаления записи
void DeleteRecord(HWND hwnd) {
    int selectedIndex = SendMessage(hListBox, LB_GETCURSEL, 0, 0);
    if (selectedIndex != LB_ERR) {
        std::istringstream iss(records[selectedIndex]);
        std::vector<std::string> words;
        std::string word;
        while(iss >> word) {
            words.push_back(word);
        }
        if(words.back() == "DELETED") {
            MessageBoxA(hwnd, "Record is already deleted!", "Error", MB_OK | MB_ICONERROR);
            return;
        }
        std::string msg = db->deleteRecord(std::stoull(words[1]));
        if(msg.length() > 0)
            MessageBoxA(hwnd, msg.c_str(), "Info", MB_OK | MB_ICONINFORMATION);
        ListRecords(hwnd);
    } else {
        MessageBoxA(hwnd, "Please select a record to delete!", "Error", MB_OK | MB_ICONERROR);
    }
}

// Процедура обработки сообщений
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    std::string buf;
    switch (message) {
    case WM_CREATE: {
        CreateWindowA("static", "Name:", WS_VISIBLE | WS_CHILD, 10, 10, 80, 30, hwnd, nullptr, nullptr, nullptr);
        hEditName = CreateWindowA("edit", "", WS_VISIBLE | WS_CHILD | WS_BORDER, 100, 10, 400, 35, hwnd, nullptr, nullptr, nullptr);

        CreateWindowA("static", "Age:", WS_VISIBLE | WS_CHILD, 10, 60, 80, 30, hwnd, nullptr, nullptr, nullptr);
        hEditAge = CreateWindowA("edit", "", WS_VISIBLE | WS_CHILD | WS_BORDER, 100, 60, 400, 35, hwnd, nullptr, nullptr, nullptr);

        CreateWindowA("button", "Add", WS_VISIBLE | WS_CHILD, 10, 130, 100, 35, hwnd, (HMENU)1, nullptr, nullptr);

        CreateWindowA("button", "Delete", WS_VISIBLE | WS_CHILD, 130, 130, 100, 35, hwnd, (HMENU)2, nullptr, nullptr);

        CreateWindowA("button", "All Entities", WS_VISIBLE | WS_CHILD, 250, 130, 100, 35, hwnd, (HMENU)3, nullptr, nullptr);

        CreateWindowA("button", "Size", WS_VISIBLE | WS_CHILD, 510, 10, 100, 35, hwnd, (HMENU)4, nullptr, nullptr);

        CreateWindowA("button", "Deleted count", WS_VISIBLE | WS_CHILD, 630, 10, 100, 35, hwnd, (HMENU)5, nullptr, nullptr);

        CreateWindowA("button", "Total count", WS_VISIBLE | WS_CHILD, 510, 60, 100, 35, hwnd, (HMENU)6, nullptr, nullptr);

        hListBox = CreateWindowA("listbox", nullptr, WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL, 10, 170, 700, 370, hwnd, nullptr, nullptr, nullptr);
        break;
    }
    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case 1: 
            AddRecord(hwnd);
            break;
        case 2: 
            DeleteRecord(hwnd);
            break;
        case 3:
            ListRecords(hwnd, true);
            break;
        case 4:
            buf = "Size: " + std::to_string(db->countRecords());
            MessageBoxA(hwnd, buf.c_str(), "Info", MB_OK | MB_ICONINFORMATION);
            break;
        case 5:
            buf = "Deleted count: " + std::to_string(db->countDeleted());
            MessageBoxA(hwnd, buf.c_str(), "Info", MB_OK | MB_ICONINFORMATION);
            break;
        case 6:
            buf = "Total count: " + std::to_string(db->countTotal());
            MessageBoxA(hwnd, buf.c_str(), "Info", MB_OK | MB_ICONINFORMATION);
            break;
        }
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}

void prepareDataBase() {
    db = new Database<Record>("test.db");
    Record r1 = {"John Doe", 25};
    Record r2 = {"Jane Smith", 30};
    Record r3 = {"Bob Smith", 40};
    Record r4 = {"Clark Kent", 35};
    Record r5 =  {"Paul Petrov", 20};
    Record r6 = {"Yarolsav Extemski", 27};
    Record r7 = {"Rostislave Gosling", 21};
    Record r8 = {"Anton Guglis", 18};
    Record r9 = {"Shersjen` Shershnev", 25};
    Record r10 = {"Stanislave Pekarev", 20};
    db->addRecord(r1);
    db->addRecord(r2);
    db->addRecord(r3);
    db->addRecord(r4);
    // db->addRecord(r5);
    // db->addRecord(r6);
    // db->addRecord(r7);
    // db->addRecord(r8);
    // db->addRecord(r9);
    // db->addRecord(r10);
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow) {
    prepareDataBase();

    WNDCLASSEXA wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "DatabaseApp";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    if (!RegisterClassExA(&wc)) {
        MessageBoxA(nullptr, "Window Registration Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    HWND hwnd = CreateWindowExA(
        0, "DatabaseApp", "Database Manager",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        nullptr, nullptr, hInstance, nullptr
    );

    if (hwnd == nullptr) {
        MessageBoxA(nullptr, "Window Creation Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    ListRecords(hwnd);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return static_cast<int>(msg.wParam);
}
