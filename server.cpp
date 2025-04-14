#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <ctime>
#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_PORT 1234 // замените на свой
#define DEFAULT_BUFLEN 512

std::vector<SOCKET> clients;
bool Running = true;

DWORD WINAPI HandleClient(LPVOID lpParam) {
    SOCKET clientSocket = (SOCKET)lpParam;
    char recvbuf[DEFAULT_BUFLEN];
    int iResult;

    while (Running) {
        iResult = recv(clientSocket, recvbuf, DEFAULT_BUFLEN, 0);
        if (iResult > 0) {
            recvbuf[iResult] = '\0';
            std::cout << "Client " << clientSocket << " sent: " << recvbuf << std::endl;

            // отправляем другим клиентам уведомление о нажатии на пробел кем-то
            for (SOCKET otherClient : clients) {
                if (otherClient != clientSocket) {
                    send(otherClient, "other_user_press", 16, 0);
                }
            }
        } else if (iResult == 0) {
            std::cout << "Client " << clientSocket << " disconnected." << std::endl;
            break;
        } else {
            std::cerr << "recv failed: " << WSAGetLastError() << std::endl;
            break;
        }
    }

    // удаляем клиента из списка после отключения
    for (auto it = clients.begin(); it != clients.end(); ++it) {
        if (*it == clientSocket) {
            clients.erase(it);
            break;
        }
    }

    closesocket(clientSocket);
    return 0;
}


// запуск

int main() {
    WSADATA wsaData;
    SOCKET ListenSocket = INVALID_SOCKET;
    struct sockaddr_in serverAddr;

    // Инициализация Winsock
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cerr << "WSAStartup failed: " << iResult << std::endl;
        return 1;
    }

    // сокет
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
    iResult = bind(ListenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    if (iResult == SOCKET_ERROR) {
        std::cerr << "bind failed: " << WSAGetLastError() << std::endl;
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // ожидание подключений
    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        std::cerr << "listen failed: " << WSAGetLastError() << std::endl;
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Server started. Waiting for connections on port " << DEFAULT_PORT << "..." << std::endl;

    // Основной цикл (ура!)
    while (Running) {
        SOCKET ClientSocket = accept(ListenSocket, NULL, NULL);
        if (ClientSocket == INVALID_SOCKET) {
            std::cerr << "accept failed: " << WSAGetLastError() << std::endl;
            continue;
        }

        std::cout << "New client connected: " << ClientSocket << std::endl;
        clients.push_back(ClientSocket);

        // Создаем поток для обработки клиента
        CreateThread(NULL, 0, HandleClient, (LPVOID)ClientSocket, 0, NULL);
    }

    // Завершение работы
    closesocket(ListenSocket);
    WSACleanup();
    return 0;
}
