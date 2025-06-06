// Что-ж, пока все работает!

#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <map>
#include <atomic>
#include <cstdlib>
#include <ctime>
#include <cstdint>


#pragma comment(lib, "ws2_32.lib")


std::mutex clientsMutex;
std::map<int, SOCKET> clients;
std::atomic<bool> running(true);
std::atomic<int> activeClient(-1);
const uint16_t PORT = 1234; // ПОМЕНЯЙТЕ НА СВОЙ ПОРТ


void Broadcast(const std::string& msg, int senderId) {
    std::lock_guard<std::mutex> lock(clientsMutex);
    for (const auto& [id, client] : clients) {
        send(client, msg.c_str(), msg.size(), 0);
    }
}

void HandleClient(SOCKET clientSocket, int clientId) {
    char buffer[512];
    while (running) {
        int recvSize = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (recvSize <= 0) break;

        buffer[recvSize] = '\0';
        std::string msg(buffer);

        if (msg == "space_down") {
            if (activeClient != clientId) {
                activeClient = clientId;
                std::cout << "Client " << clientId << " talking...\n";
                Broadcast("space_down", clientId);
            }
        } else if (msg == "space_up") {
            if (activeClient == clientId) {
                activeClient = -1;
                Broadcast("space_up", clientId);
            }
        }
        std::cout << "Received: " << msg << " from Client " << clientId << "\n";
    }

    std::lock_guard<std::mutex> lock(clientsMutex);
    closesocket(clientSocket);
    clients.erase(clientId);
    std::cout << "Client " << clientId << " disconnected.\n";
}

void ListenForEscape() {
    while (running) {
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            running = false;
            break;
        }
        Sleep(100);
    }
}

int main() {
    setlocale(LC_ALL, "RU"); // локализировал консоль чтобы не было нечитаемого текста
    SetConsoleOutputCP(CP_UTF8);      // консоль — на UTF-8 (иначе тоже будет нечитаемый текст)
    SetConsoleCP(CP_UTF8);            // ввод — тоже



    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Не получилось забиндить! Проверьте, работает ли на этом порту что-то другое (netstat -ano | findstr :<port>)" << std::endl;
        Sleep(2000);
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Не получилось прослушать! Проверьте, работает ли на этом порту что-то другое (netstat -ano | findstr :<port>)" << std::endl;
        Sleep(2000);
        return 1;
    }

    std::cout << "Ожидание подключения...\n";
    srand((unsigned)time(nullptr));

    std::thread escThread(ListenForEscape);

    fd_set readfds;
    struct timeval timeout;

    while (running) {
        FD_ZERO(&readfds);
        FD_SET(serverSocket, &readfds);

        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int activity = select(0, &readfds, nullptr, nullptr, &timeout);

        if (activity > 0 && FD_ISSET(serverSocket, &readfds)) {
            sockaddr_in clientAddr;
            int clientSize = sizeof(clientAddr);
            SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientSize);
            if (!running) break;

            int clientId = rand() % 99 + 1;
            {
                std::lock_guard<std::mutex> lock(clientsMutex);
                clients[clientId] = clientSocket;
            }

            std::cout << "Клиент № " << clientId << " подключился!\n";
            std::thread(HandleClient, clientSocket, clientId).detach();
        }

        if (!running) break;
    }

    std::cout << "Выключение сервера...\n";
    Sleep(500);
    closesocket(serverSocket);
    WSACleanup();
    escThread.join();
    return 0;
}

