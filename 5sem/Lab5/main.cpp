#include "client.cpp"
#include <windows.h>
#include <string>

HWND hInput, hOutput;
Client* client = nullptr;

// Функция для получения ответа от сервера и отображения в GUI
void receiveResponse() {
    std::string response = client->getResponse();
    SetWindowTextA(hOutput, response.c_str());
}

// Процедура окна
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	std::string error;
	std::string buf;
    switch (uMsg) {
    case WM_DESTROY:
        client->shutdown();
        PostQuitMessage(0);
        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == 1) {
            char message[4096];
            GetWindowTextA(hInput, message, sizeof(message));
			buf = std::string(message);
			if(!buf.length()) break;
			error = client->sendRequest(buf);
            if (!error.length()) {
                receiveResponse();
            } else {
				SetWindowTextA(hOutput, error.c_str());
			}
        }
        break;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

// Главная функция
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    freopen("CONIN$", "r", stdin);

    std::string serverIP = "127.0.0.1";
    int serverPort;

    std::cout << "Enter server port: ";
    std::cin >> serverPort;
    std::cin.ignore();
    FreeConsole();

    client = new Client(serverIP, serverPort);
    if (!client->connectToServer()) {
        MessageBoxA(NULL, "Failed to connect to the server", "Error", MB_OK);
        return 1;
    }

    const char CLASS_NAME[] = "MorseClient";
    WNDCLASSA wc = { };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClassA(&wc);

    HWND hwnd = CreateWindowExA(
        0, CLASS_NAME, "Morse Code Client", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 300,
        NULL, NULL, hInstance, NULL
    );

    hInput = CreateWindowExA(0, "EDIT", "",
                            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                            20, 20, 500, 25,
                            hwnd, NULL, hInstance, NULL);

    CreateWindowExA(0, "BUTTON", "Send",
                   WS_CHILD | WS_VISIBLE,
                   490, 60, 80, 30,
                   hwnd, (HMENU)1, hInstance, NULL);

    hOutput = CreateWindowExA(0, "EDIT", "",
                             WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL,
                             20, 100, 500, 150,
                             hwnd, NULL, hInstance, NULL);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    delete client;
    return 0;
}

//g++ -o main.exe main.cpp -lws2_32 -mwindows
