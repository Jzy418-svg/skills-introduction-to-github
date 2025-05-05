#include <winsock2.h>
#include <windows.h>
#include <string>
#include <sstream>

#pragma comment(lib, "ws2_32.lib") // ���� Winsock2 ��

// ȫ�ֱ���
HINSTANCE g_hInst;
HWND g_hWndEditInput, g_hWndEditChat, g_hWndButtonSend, g_hWndButtonConnect, g_hWndEditIP, g_hWndEditPort;
SOCKET sock = INVALID_SOCKET;
bool isConnected = false;

// ��ʾ��Ϣ�������
void DisplayMessage(const std::string& msg) {
    char chatBuffer[1024];
    GetWindowText(g_hWndEditChat, chatBuffer, sizeof(chatBuffer));

    std::ostringstream oss;
    oss << chatBuffer << msg << "\r\n";
    SetWindowText(g_hWndEditChat, oss.str().c_str());
}

// ������Ϣ�߳�
DWORD WINAPI ReceiveMessages(LPVOID lpParam) {
    char buffer[256];
    while (isConnected) {
        int ret = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (ret > 0) {
            buffer[ret] = '\0';
            DisplayMessage("�Է�: " + std::string(buffer));
        } else {
            DisplayMessage("�����ѶϿ���");
            isConnected = false;
            break;
        }
    }
    return 0;
}

// ���ӵ�������
void ConnectToServer(const std::string& ip, int port) {
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(ip.c_str());
    serverAddr.sin_port = htons(port);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        MessageBox(NULL, "�����׽���ʧ�ܣ�", "����", MB_ICONERROR);
        return;
    }

    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        MessageBox(NULL, "���ӷ�����ʧ�ܣ�", "����", MB_ICONERROR);
        closesocket(sock);
        sock = INVALID_SOCKET;
        return;
    }

    isConnected = true;
    DisplayMessage("�ɹ����ӵ���������");
    CreateThread(NULL, 0, ReceiveMessages, NULL, 0, NULL);
}

// ����������
void StartServer(int port) {
    struct sockaddr_in serverAddr, clientAddr;
    int addrLen = sizeof(clientAddr);

    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSock == INVALID_SOCKET) {
        MessageBox(NULL, "���������׽���ʧ�ܣ�", "����", MB_ICONERROR);
        return;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(listenSock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        MessageBox(NULL, "�󶨶˿�ʧ�ܣ�", "����", MB_ICONERROR);
        closesocket(listenSock);
        return;
    }

    if (listen(listenSock, 1) == SOCKET_ERROR) {
        MessageBox(NULL, "����ʧ�ܣ�", "����", MB_ICONERROR);
        closesocket(listenSock);
        return;
    }

    DisplayMessage("�ȴ��ͻ�������...");
    sock = accept(listenSock, (struct sockaddr*)&clientAddr, &addrLen);
    if (sock == INVALID_SOCKET) {
        MessageBox(NULL, "��������ʧ�ܣ�", "����", MB_ICONERROR);
        closesocket(listenSock);
        return;
    }

    isConnected = true;
    DisplayMessage("�ͻ��������ӣ�");
    CreateThread(NULL, 0, ReceiveMessages, NULL, 0, NULL);
    closesocket(listenSock);
}

// ������Ϣ
void SendMessageToPeer() {
    char buffer[256];
    GetWindowText(g_hWndEditInput, buffer, sizeof(buffer));
    if (strlen(buffer) > 0) {
        send(sock, buffer, strlen(buffer), 0);
        DisplayMessage("��: " + std::string(buffer));
        SetWindowText(g_hWndEditInput, "");
    }
}

// ���ڹ��̺���
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        // �����¼��
        g_hWndEditChat = CreateWindow(
            "EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | ES_READONLY,
            10, 10, 460, 300, hwnd, (HMENU)1, g_hInst, NULL);

        // �����
        g_hWndEditInput = CreateWindow(
            "EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            10, 320, 300, 30, hwnd, (HMENU)2, g_hInst, NULL);

        // IP�����
        g_hWndEditIP = CreateWindow(
            "EDIT", "127.0.0.1", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            320, 320, 150, 30, hwnd, (HMENU)3, g_hInst, NULL);

        // �˿������
        g_hWndEditPort = CreateWindow(
            "EDIT", "8080", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            320, 360, 150, 30, hwnd, (HMENU)4, g_hInst, NULL);

        // ���Ӱ�ť
        g_hWndButtonConnect = CreateWindow(
            "BUTTON", "����/����", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            320, 400, 150, 30, hwnd, (HMENU)5, g_hInst, NULL);

        // ���Ͱ�ť
        g_hWndButtonSend = CreateWindow(
            "BUTTON", "����", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            10, 360, 300, 30, hwnd, (HMENU)6, g_hInst, NULL);
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == 5) { // ����/������ť
            char ip[16], portStr[6];
            GetWindowText(g_hWndEditIP, ip, sizeof(ip));
            GetWindowText(g_hWndEditPort, portStr, sizeof(portStr));
            int port = atoi(portStr);

            if (!isConnected) {
                StartServer(port); // ����������
            } else {
                ConnectToServer(ip, port); // ���ӵ�������
            }
        } else if (LOWORD(wParam) == 6) { // ���Ͱ�ť
            SendMessageToPeer();
        }
        break;

    case WM_DESTROY:
        isConnected = false;
        if (sock != INVALID_SOCKET) closesocket(sock);
        WSACleanup();
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// ������
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    g_hInst = hInstance;

    // ��ʼ�� Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        MessageBox(NULL, "��ʼ�� Winsock ʧ�ܣ�", "����", MB_ICONERROR);
        return 0;
    }

    // ע�ᴰ����
    WNDCLASS wc = {};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "ChatAppClass";

    if (!RegisterClass(&wc)) {
        MessageBox(NULL, "����ע��ʧ�ܣ�", "����", MB_ICONERROR);
        return 0;
    }

    // ����������
    HWND hwnd = CreateWindow(
        "ChatAppClass", "�����������", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 500,
        NULL, NULL, hInstance, NULL);

    if (!hwnd) {
        MessageBox(NULL, "���ڴ���ʧ�ܣ�", "����", MB_ICONERROR);
        return 0;
    }

    // ��ʾ����
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // ��Ϣѭ��
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}
