#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <algorithm>
#include <commdlg.h>

// Структура для хранения задания
struct Task {
    int hour;
    int minute;
    std::string executable;
    std::vector<std::string> parameters;
    std::string status = "Pending";  // (Pending, Running, Completed, Failed)
};

// Глобальные переменные для окна
HWND hMainWnd;
std::vector<Task> tasks;

std::string convertTime(unsigned short time) {
    std::string converted = std::to_string(time);
    return time < 10 ? ("0" + converted) : converted;
}

// Функция для сортировки задач по времени
void sortTasksByTime(std::vector<Task>& tasks) {
    std::sort(tasks.begin(), tasks.end(), [](const Task& a, const Task& b) {
        if (a.hour == b.hour) {
            return a.minute < b.minute; // Сравниваем по минутам, если часы равны
        }
        return a.hour < b.hour; // Сравниваем по часам
    });
}

// Функция для открытия диалога выбора файла
std::string openFileDialog() {
    OPENFILENAMEA ofn;   
    char filename[MAX_PATH] = ""; 
    ZeroMemory(&ofn, sizeof(ofn)); 
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFilter = "Config Files\0*.txt\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = "txt";

    if (GetOpenFileNameA(&ofn)) {
        return std::string(filename);
    } else {
        return "";
    }
}

// Функция для записи логов
void writeLog(const std::string& message) {
    std::ofstream logFile("cron_log.txt", std::ios_base::app);
    if (logFile.is_open()) {
        SYSTEMTIME time;
        GetLocalTime(&time);
        logFile << "[" << convertTime(time.wHour) << ":" << convertTime(time.wMinute) << ":"
                << convertTime(time.wSecond) << "] " << message << "\n";
        logFile.close();
    }
}

// Функция для чтения конфигурационного файла
std::vector<Task> readConfig(const std::string& filename) {
    std::vector<Task> tasks;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << "\n";
        writeLog("Error opening config file: " + filename);
        return tasks;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        Task task;
        char colon;
        iss >> task.hour >> colon >> task.minute; // Читаем время
        std::string buffer;
        iss >> std::quoted(buffer); // Читаем путь к исполняемому файлу
        task.executable = buffer;

        buffer.clear();
        while (iss >> std::quoted(buffer)) {
            task.parameters.push_back(buffer); // Читаем параметры
        }

        tasks.push_back(task);
    }

    file.close();
    writeLog("Configuration loaded successfully");
    return tasks;
}

// Функция для сравнения текущего времени с временем задачи
bool isTimeToRun(const SYSTEMTIME& currentTime, const Task& task) {
    return (currentTime.wHour == task.hour && currentTime.wMinute == task.minute);
}

// Функция для запуска программы
void runTask(Task& task) {
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    std::string commandLine = task.executable; // Кавычки для пути

    // Добавляем параметры
    for (const auto& param : task.parameters) {
        commandLine += " \"" + param + "\""; // Кавычки для параметров
    }

    // Преобразуем строку команды к формату char* для CreateProcess
    char* cmd = new char[commandLine.size() + 1];
    strcpy(cmd, commandLine.c_str());

    // Логируем запуск
    writeLog("Starting task: " + task.executable + " with parameters: " + commandLine);

    task.status = "Running";
    InvalidateRect(hMainWnd, NULL, TRUE); // Обновляем содержимое окна

    // Запускаем процесс
    if (!CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        DWORD error = GetLastError();
        std::cerr << "Failed to start process: " << task.executable << " with error " << error << std::endl;
        writeLog("Failed to start process: " + task.executable + " with error " + std::to_string(error));
        task.status = "Failed";
        delete[] cmd;
        InvalidateRect(hMainWnd, NULL, TRUE); // Обновляем содержимое окна
        return;
    }

    // Ждем завершения процесса
    WaitForSingleObject(pi.hProcess, INFINITE);

    // Закрываем дескрипторы процесса
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    writeLog("Task completed successfully: " + task.executable);
    task.status = "Completed";
    InvalidateRect(hMainWnd, NULL, TRUE); // Обновляем содержимое окна

    delete[] cmd;
}

// Функция для обновления содержимого окна
void UpdateWindowContent(HDC hdc) {
    RECT rect;
    GetClientRect(hMainWnd, &rect);
    
    std::string text = "Scheduled tasks:\n\n";

    for (const auto& task : tasks) {
        text += "Task: " + task.executable + " at " + convertTime(task.hour) + ":" + convertTime(task.minute);
        text += " - Status: " + task.status + "\n\n";
    }

    DrawTextA(hdc, text.c_str(), -1, &rect, DT_LEFT | DT_TOP);
}

// Основная функция окна
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        UpdateWindowContent(hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// Функция для создания и запуска окна
int CreateMainWindow(HINSTANCE hInstance, int nCmdShow) {
    const char CLASS_NAME[] = "SchedulerWindow";

    WNDCLASSA wc = { };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClassA(&wc);

    hMainWnd = CreateWindowExA(
        0,
        CLASS_NAME,
        "Scheduler Window",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 500,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hMainWnd == NULL) {
        return 0;
    }

    ShowWindow(hMainWnd, nCmdShow);

    return 1;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {  
    std::string configPath = openFileDialog();

    if (configPath.empty()) {
        std::cerr << "No file selected. Exiting." << std::endl;
        writeLog("No file selected. Exiting.");
        return 1;
    }
    
    std::cout << "Reading schedule from: " << configPath << "\n";
    // Читаем конфигурацию
    tasks = readConfig("cron_config.txt");

    if (tasks.empty()) {
        std::cerr << "No tasks to run. Exiting." << "\n";
        writeLog("No tasks to run. Exiting.");
        return 1;
    }

    // Сортируем задачи по времени выполнения
    sortTasksByTime(tasks);

    const int task_count = tasks.size();
    int counter = 0;

    // Создаем главное окно
    if (!CreateMainWindow(hInstance, nCmdShow)) {
        return 0;
    }

    // Основной цикл сообщений
    MSG msg = { };
    writeLog("Scheduler started");

    while (counter != task_count) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            SYSTEMTIME currentTime;
            GetLocalTime(&currentTime);

            for (auto& task : tasks) {
                if (task.status == "Pending" && isTimeToRun(currentTime, task)) {
                    std::cout << "Running task: " << task.executable << "\n";
                    runTask(task);
                    ++counter;
                }
            }

            Sleep(1000); // Ожидаем 1 секунду перед новой проверкой
        }
    }

    writeLog("All tasks completed\n---------------\n");
    return 0;
}
