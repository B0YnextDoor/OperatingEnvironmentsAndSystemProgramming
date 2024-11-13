#include "regedit.cpp"

Logger *logger = new Logger("clean_log.log", "backup.txt");
RegEdit *cleaner = new RegEdit(logger);

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_COMMAND:
		if (LOWORD(wParam) == 1)
		{
			if (logger->log("Cleaning started."))
				break;
			if (!cleaner->cleanRegistry(HKEY_CURRENT_USER, "Software\\Test"))
				logger->log("Cleaning finished.");
			logger->close();
		}
		else if (LOWORD(wParam) == 2)
		{
			SetWindowTextA(logger->logEdit, "");
			if (logger->log("Backup started."))
				break;
			cleaner->restoreBackup();
			logger->close();
		}
		break;
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
	return 0;
}

// Основная функция
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	const char className[] = "RegistryCleaner";
	WNDCLASSA wc = {};
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = className;

	RegisterClassA(&wc);

	HWND hwnd = CreateWindowExA(0, className, "Regisrty cleaner", WS_OVERLAPPEDWINDOW,
								CW_USEDEFAULT, CW_USEDEFAULT, 550, 550,
								NULL, NULL, hInstance, NULL);

	if (!hwnd)
	{
		MessageBoxA(NULL, "Error creating the window!", "Error", MB_OK);
		return 1;
	}

	CreateWindowExA(0, "BUTTON", "Clear registry", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 50, 50, 150, 30, hwnd, (HMENU)1, hInstance, NULL);
	CreateWindowA("BUTTON", "Backup", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 300, 50, 150, 30, hwnd, (HMENU)2, hInstance, NULL);
	logger->logEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
									  WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL,
									  10, 100, 520, 400, hwnd, NULL, hInstance, NULL);

	ShowWindow(hwnd, nShowCmd);
	UpdateWindow(hwnd);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	delete logger;
	delete cleaner;
	return 0;
}
