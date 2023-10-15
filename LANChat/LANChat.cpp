#include <winsock2.h>
#include <richedit.h>
#include <windows.h>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

#define ID_EDIT 101
#define ID_BUTTON 102

HWND hEdit;
HWND hButton;
HWND hMessageDisplay;

SOCKET s;
sockaddr_in server, client;

void ListenForMessages() {
    wchar_t buffer[1024];
    int len = sizeof(client);

    while (true) {
        ZeroMemory(buffer, sizeof(buffer));
        int bytesReceived = recvfrom(s, (char*)buffer, sizeof(buffer), 0, (sockaddr*)&client, &len);
        if (bytesReceived != SOCKET_ERROR) {
            buffer[bytesReceived / sizeof(wchar_t)] = L'\0'; // Null-terminate the received wide string
            wcscat_s(buffer, L"\r\n");
            SendMessage(hMessageDisplay, EM_REPLACESEL, 0, (LPARAM)buffer);
        }
    }
}



LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        LoadLibrary(TEXT("Msftedit.dll"));
        hMessageDisplay = CreateWindowEx(WS_EX_CLIENTEDGE, MSFTEDIT_CLASS, L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY, 10, 10, 260, 180, hwnd, NULL, GetModuleHandle(NULL), NULL);
        hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE, 10, 200, 260, 25, hwnd, (HMENU)ID_EDIT, GetModuleHandle(NULL), NULL);
        hButton = CreateWindowEx(0, L"BUTTON", L"Send", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 10, 230, 260, 30, hwnd, (HMENU)ID_BUTTON, GetModuleHandle(NULL), NULL);
        break;
    }
    case WM_COMMAND: {
        if (LOWORD(wParam) == ID_BUTTON) {
            wchar_t message[1024];
            GetWindowText(hEdit, message, sizeof(message) / sizeof(wchar_t));
            sendto(s, (char*)message, wcslen(message) * sizeof(wchar_t), 0, (sockaddr*)&server, sizeof(server));
        }
        break;
    }
    case WM_CLOSE:
        closesocket(s);
        WSACleanup();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSEX wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"LANChatClass";

    RegisterClassEx(&wc);

    HWND hwnd = CreateWindowEx(0, L"LANChatClass", L"LAN Chat", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 300, 300, NULL, NULL, hInstance, NULL);
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    s = socket(AF_INET, SOCK_DGRAM, 0);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_BROADCAST;
    server.sin_port = htons(8888);

    int optval = 1;
    setsockopt(s, SOL_SOCKET, SO_BROADCAST, (char*)&optval, sizeof(optval));

    client.sin_family = AF_INET;
    client.sin_addr.s_addr = INADDR_ANY;
    client.sin_port = htons(8888);
    bind(s, (sockaddr*)&client, sizeof(client));

    std::thread t(ListenForMessages);
    t.detach();

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg.wParam;
}
