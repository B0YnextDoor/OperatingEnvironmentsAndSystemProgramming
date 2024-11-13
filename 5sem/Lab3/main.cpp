#include "process.cpp"

// Глобальные переменные для разделяемой памяти и мьютекса
SharedMemory* shared_memory;
Mutex* mutex;
HWND hEditThreadCount; // Поле ввода для количества потоков

// Функция обратного вызова (Callback) для обработки сообщений окна
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        // Создание кнопок и текстового поля для вывода
        hEditThreadCount = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "3", 
            WS_CHILD | WS_VISIBLE | ES_NUMBER, 
            10, 10, 50, 30, hWnd, NULL, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
        CreateWindowA("BUTTON", "Start Process", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            80, 10, 120, 30, hWnd, (HMENU)1, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
        CreateWindowA("BUTTON", "Stop All", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            220, 10, 120, 30, hWnd, (HMENU)2, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
        
        hEditOutput = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", 
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL, 
            10, 50, 650, 550, hWnd, NULL, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == 1) {
            char buffer[10];
            GetWindowTextA(hEditThreadCount, buffer, sizeof(buffer));
            int threadCount = 0; // Переменная для хранения количества потоков
            try {
                // Проверка, не является ли введенное значение пустым
                if (buffer[0] == '\0') {
                    SetWindowTextA(hEditOutput, "Please enter a valid number.\r\n");
                    break;
                }

                threadCount = std::stoi(buffer); // Преобразование строки в целое число

            } catch (const std::invalid_argument&) {
                SetWindowTextA(hEditOutput, "Invalid input: not a number.\r\n");
                return 0;
            } catch (const std::out_of_range&) {
                SetWindowTextA(hEditOutput, "Invalid input: number too large.\r\n");
                return 0;
            }
            SetWindowTextA(hEditOutput, "");
            // Запуск процессов по нажатию кнопки
            for (int i = 0; i < threadCount; ++i) {
                threads.emplace_back(UserProcess(i + 1, *shared_memory, *mutex));
            }
            break;
        }
        else if(LOWORD(wParam) == 2) {
            UserProcess(-1, *shared_memory, *mutex).StopAllProcesses();
            for (auto& th : threads) {
                if (th.joinable())
                    th.join();  // Ожидаем завершения потока
            }
            threads.clear(); // Очищаем вектор потоков
            break;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Точка входа в приложение
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Инициализация разделяемой памяти и мьютекса
    shared_memory = new SharedMemory("MySharedMemory");
    mutex = new Mutex("MyMutex");

    WNDCLASSA wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "SharedMemoryApp";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.style = CS_HREDRAW | CS_VREDRAW;

    RegisterClassA(&wc);

    // Создание окна
    HWND hWnd = CreateWindowA("SharedMemoryApp", "Shared Memory", 
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 700, 650, 
        NULL, NULL, hInstance, NULL);

    if (!hWnd) {
        return 0;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // Основной цикл сообщений
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Освобождение ресурсов
    delete shared_memory;
    delete mutex;

    return (int)msg.wParam;
}
