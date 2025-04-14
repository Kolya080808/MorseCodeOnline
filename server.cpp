#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <atomic>
#include <algorithm>
#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_PORT 1234
#define DEFAULT_BUFLEN 512

std::vector<SOCKET> clients;
std::atomic<bool> Running(true);

// Функция для обработки клиента
DWORD WINAPI HandleClient(LPVOID lpParam) {
    SOCKET clientSocket = (SOCKET)lpParam;
    char recvbuf[DEFAULT_BUFLEN];
    int iResult;

    while (Running) {
        iResult = recv(clientSocket, recvbuf, DEFAULT_BUFLEN, 0);
        if (iResult > 0) {
            recvbuf[iResult] = '\0';
            std::cout << "Client " << clientSocket << " sent: " << recvbuf << std::endl;

            // Отправляем уведомление другим клиентам
            for (SOCKET otherClient : clients) {
                if (otherClient != clientSocket && Running) {
                    send(otherClient, "other_user_press", 16, 0);
                }
            }
        }
        else if (iResult == 0) {
            std::cout << "Client " << clientSocket << " disconnected." << std::endl;
            break;
        }
        else {
            std::cerr << "recv failed: " << WSAGetLastError() << std::endl;
            break;
        }
    }

    // Удаляем клиента из списка
    auto it = std::find(clients.begin(), clients.end(), clientSocket);
    if (it != clients.end()) {
        clients.erase(it);
    }
    closesocket(clientSocket);
    return 0;
}

// Поток для отслеживания нажатия клавиши Q
DWORD WINAPI ShutdownListener(LPVOID lpParam) {
    (void)lpParam;
    while (Running) {
        if (GetAsyncKeyState('Q') & 0x8000) {
            std::cout << "\nInitiating server shutdown..." << std::endl;
            Running = false;
            
            // Закрываем сокет прослушивания чтобы выйти из accept()
            closesocket(*(SOCKET*)lpParam);
            break;
        }
        Sleep(100);
    }
    return 0;
}

int main() {
    WSADATA wsaData;
    SOCKET ListenSocket = INVALID_SOCKET;
    struct sockaddr_in serverAddr;

    // Инициализация Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }

    // Создание сокета
    ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ListenSocket == INVALID_SOCKET) {
        std::cerr << "socket failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    // Настройка адреса
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(DEFAULT_PORT);

    // Привязка сокета
    if (bind(ListenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "bind failed: " << WSAGetLastError() << std::endl;
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // Ожидание подключений
    if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "listen failed: " << WSAGetLastError() << std::endl;
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Server started. Press Q to shutdown..." << std::endl;

    // Запускаем поток для отслеживания клавиши Q
    HANDLE hShutdownThread = CreateThread(NULL, 0, ShutdownListener, &ListenSocket, 0, NULL);

    // Основной цикл
    while (Running) {
        SOCKET ClientSocket = accept(ListenSocket, NULL, NULL);
        if (ClientSocket == INVALID_SOCKET) {
            if (Running) { // Если это не запланированное закрытие
                std::cerr << "accept failed: " << WSAGetLastError() << std::endl;
            }
            continue;
        }

        std::cout << "New client connected: " << ClientSocket << std::endl;
        clients.push_back(ClientSocket);
        CreateThread(NULL, 0, HandleClient, (LPVOID)ClientSocket, 0, NULL);
    }

    // Корректное завершение работы
    std::cout << "Closing all connections..." << std::endl;

    // Закрываем все клиентские сокеты
    for (SOCKET s : clients) {
        shutdown(s, SD_BOTH);
        closesocket(s);
    }
    clients.clear();

    // Ожидаем завершение потока shutdown listener
    WaitForSingleObject(hShutdownThread, INFINITE);
    CloseHandle(hShutdownThread);

    // Финализация Winsock
    closesocket(ListenSocket);
    WSACleanup();

    std::cout << "Server shutdown complete." << std::endl;
    return 0;
}
