#include <winsock2.h>
#include <windows.h>
#include <string>
#include <sstream>

#pragma comment(lib, "ws2_32.lib") // 链接 Winsock2 库

// 全局变量
HINSTANCE g_hInst;
HWND g_hWndEditInput, g_hWndEditChat, g_hWndButtonSend, g_hWndButtonConnect, g_hWndEditIP, g_hWndEditPort;
SOCKET sock = INVALID_SOCKET;
bool isConnected = false;

// 显示消息到聊天框
void DisplayMessage(const std::string& msg) {
    char chatBuffer[1024];
    GetWindowText(g_hWndEditChat, chatBuffer, sizeof(chatBuffer));

    std::ostringstream oss;
    oss << chatBuffer << msg << "\r\n";
    SetWindowText(g_hWndEditChat, oss.str().c_str());
}

// 接收消息线程
DWORD WINAPI ReceiveMessages(LPVOID lpParam) {
    char buffer[256];
    while (isConnected) {
        int ret = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (ret > 0) {
            buffer[ret] = '\0';
            DisplayMessage("对方: " + std::string(buffer));
        } else {
            DisplayMessage("连接已断开！");
            isConnected = false;
            break;
        }
    }
    return 0;
}

// 连接到服务器
void ConnectToServer(const std::string& ip, int port) {
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(ip.c_str());
    serverAddr.sin_port = htons(port);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        MessageBox(NULL, "创建套接字失败！", "错误", MB_ICONERROR);
        return;
    }

    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        MessageBox(NULL, "连接服务器失败！", "错误", MB_ICONERROR);
        closesocket(sock);
        sock = INVALID_SOCKET;
        return;
    }

    isConnected = true;
    DisplayMessage("成功连接到服务器！");
    CreateThread(NULL, 0, ReceiveMessages, NULL, 0, NULL);
}

// 启动服务器
void StartServer(int port) {
    struct sockaddr_in serverAddr, clientAddr;
    int addrLen = sizeof(clientAddr);

    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSock == INVALID_SOCKET) {
        MessageBox(NULL, "创建监听套接字失败！", "错误", MB_ICONERROR);
        return;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(listenSock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        MessageBox(NULL, "绑定端口失败！", "错误", MB_ICONERROR);
        closesocket(listenSock);
        return;
    }

    if (listen(listenSock, 1) == SOCKET_ERROR) {
        MessageBox(NULL, "监听失败！", "错误", MB_ICONERROR);
        closesocket(listenSock);
        return;
    }

    DisplayMessage("等待客户端连接...");
    sock = accept(listenSock, (struct sockaddr*)&clientAddr, &addrLen);
    if (sock == INVALID_SOCKET) {
        MessageBox(NULL, "接受连接失败！", "错误", MB_ICONERROR);
        closesocket(listenSock);
        return;
    }

    isConnected = true;
    DisplayMessage("客户端已连接！");
    CreateThread(NULL, 0, ReceiveMessages, NULL, 0, NULL);
    closesocket(listenSock);
}

// 发送消息
void SendMessageToPeer() {
    char buffer[256];
    GetWindowText(g_hWndEditInput, buffer, sizeof(buffer));
    if (strlen(buffer) > 0) {
        send(sock, buffer, strlen(buffer), 0);
        DisplayMessage("你: " + std::string(buffer));
        SetWindowText(g_hWndEditInput, "");
    }
}

// 窗口过程函数
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        // 聊天记录框
        g_hWndEditChat = CreateWindow(
            "EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | ES_READONLY,
            10, 10, 460, 300, hwnd, (HMENU)1, g_hInst, NULL);

        // 输入框
        g_hWndEditInput = CreateWindow(
            "EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            10, 320, 300, 30, hwnd, (HMENU)2, g_hInst, NULL);

        // IP输入框
        g_hWndEditIP = CreateWindow(
            "EDIT", "127.0.0.1", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            320, 320, 150, 30, hwnd, (HMENU)3, g_hInst, NULL);

        // 端口输入框
        g_hWndEditPort = CreateWindow(
            "EDIT", "8080", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            320, 360, 150, 30, hwnd, (HMENU)4, g_hInst, NULL);

        // 连接按钮
        g_hWndButtonConnect = CreateWindow(
            "BUTTON", "连接/启动", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            320, 400, 150, 30, hwnd, (HMENU)5, g_hInst, NULL);

        // 发送按钮
        g_hWndButtonSend = CreateWindow(
            "BUTTON", "发送", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            10, 360, 300, 30, hwnd, (HMENU)6, g_hInst, NULL);
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == 5) { // 连接/启动按钮
            char ip[16], portStr[6];
            GetWindowText(g_hWndEditIP, ip, sizeof(ip));
            GetWindowText(g_hWndEditPort, portStr, sizeof(portStr));
            int port = atoi(portStr);

            if (!isConnected) {
                StartServer(port); // 启动服务器
            } else {
                ConnectToServer(ip, port); // 连接到服务器
            }
        } else if (LOWORD(wParam) == 6) { // 发送按钮
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

// 主函数
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    g_hInst = hInstance;

    // 初始化 Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        MessageBox(NULL, "初始化 Winsock 失败！", "错误", MB_ICONERROR);
        return 0;
    }

    // 注册窗口类
    WNDCLASS wc = {};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "ChatAppClass";

    if (!RegisterClass(&wc)) {
        MessageBox(NULL, "窗口注册失败！", "错误", MB_ICONERROR);
        return 0;
    }

    // 创建主窗口
    HWND hwnd = CreateWindow(
        "ChatAppClass", "网络聊天软件", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 500,
        NULL, NULL, hInstance, NULL);

    if (!hwnd) {
        MessageBox(NULL, "窗口创建失败！", "错误", MB_ICONERROR);
        return 0;
    }

    // 显示窗口
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // 消息循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}
