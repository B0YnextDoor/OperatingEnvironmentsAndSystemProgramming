#include "stage.cpp"
#define TIMER_ID 1

// Глобальные переменные для окна
HINSTANCE hInst;
HWND hwnd;

// Отображение состояний каналов
void draw_channels(HDC hdc, RECT& rect) {
    std::lock_guard<std::mutex> lock(state_mutex);

    int channel_height = 30;
    int channel_width = rect.right - rect.left - 20;

    for (size_t i = 0; i < channel_states.size(); i++) {
        RECT channel_rect = {10, static_cast<LONG>(110 + i * (channel_height + 10)), 
                             channel_width, static_cast<LONG>(110 + i * (channel_height + 10) + channel_height)};
        HBRUSH brush = CreateSolidBrush(channel_states[i] == 0 ? RGB(0, 255, 0) : RGB(255, 0, 0));
        FillRect(hdc, &channel_rect, brush);
        DeleteObject(brush);
        char text[20];
        sprintf_s(text, "Channel %d", static_cast<int>(i + 1));
        DrawTextA(hdc, text, -1, &channel_rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
}

// Отображение статистики
void draw_stats(HDC hdc, RECT& rect) {
    std::string stats_text = "Processed requests: " + std::to_string(stats.processed_requests) +
                  "\nLost requests: " + std::to_string(stats.lost_requests) +
                  "\nTotal processing time: " + std::to_string(int(stats.total_processing_time) / 1000) + " s";
    DrawTextA(hdc, stats_text.c_str(), -1, &rect, DT_LEFT | DT_TOP);
}

// Обработчик сообщений окна
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rect;
        GetClientRect(hwnd, &rect);
        RECT stats_rect = { 10, 10, rect.right - 20, 100 };
        draw_stats(hdc, stats_rect);
        draw_channels(hdc, rect);
        EndPaint(hwnd, &ps);
        break;
    }
    case WM_TIMER: {
        InvalidateRect(hwnd, NULL, TRUE); // Обновляем окно
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// Инициализация окна
ATOM RegisterWindowClass(HINSTANCE hInstance) {
    WNDCLASSEXA wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0, 0, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, "SmoWindowClass", NULL };
    return RegisterClassExA(&wc);
}

BOOL InitWindowInstance(HINSTANCE hInstance, int nCmdShow) {
    hwnd = CreateWindowA("SmoWindowClass", "SMO", WS_OVERLAPPEDWINDOW,
                        CW_USEDEFAULT, CW_USEDEFAULT, 900, 1024, NULL, NULL, hInstance, NULL);
    if (!hwnd) {
        return FALSE;
    }
    hListBox = CreateWindowExA(WS_EX_CLIENTEDGE, "LISTBOX", NULL, 
                                WS_CHILD | WS_VISIBLE | LBS_NOTIFY | LBS_SORT | WS_VSCROLL,
                                10, 350, 860, 500, hwnd, NULL, hInst, NULL);
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    return TRUE;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    RegisterWindowClass(hInstance);
    if (!InitWindowInstance(hInstance, nCmdShow)) {
        return FALSE;
    }
    // Настройка модели СМО
    int num_stages = 2; // Количество стадий
    int num_channels_per_stage = 3;
    int buffer_capacity = 5;
    int request_intensity = 1000; // Интервал между заявками
    int processing_time = 3000; // Время обработки заявки

    // Создаем буферы для каждой ступени
    Buffer buffer1(buffer_capacity);  // Входной буфер для первой ступени
    Buffer buffer2(buffer_capacity);  // Входной буфер для второй ступени (выходной для первой)

    // Создаем ступени
    Stage stage1(num_channels_per_stage, processing_time, buffer1, &buffer2);  // Первая ступень с выходом в buffer2
    Stage stage2(num_channels_per_stage, processing_time, buffer2, nullptr);   // Вторая ступень, последняя, нет выхода

    // Инициализация глобальных состояний каналов
    channel_states.resize(num_channels_per_stage * num_stages, 0);  // Изначально все каналы свободны

    // Запуск генератора заявок
    std::thread generator(request_generator, std::ref(buffer1), request_intensity, 15);

    // Запускаем таймер для обновления окна
    SetTimer(hwnd, TIMER_ID, 1000, NULL);

    // Цикл сообщений
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Ожидание завершения генератора заявок
    generator.join();

    return static_cast<int>(msg.wParam);
}