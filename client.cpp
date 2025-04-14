/* Я попросил чатгпт мне подправить код, так что он добавил атомарную переменную, проверку ошибок, graceful shutdown.
   Оригинальный код я так же оставил. */

/* **********************************КОД ЧАТГПТ********************************** */
#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <chrono>
#include <atomic>
#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "91.223.90.26"  // Ваш IP сервера
#define SERVER_PORT 1234
#define DEFAULT_BUFLEN 512

std::atomic<bool> Running(true);  // Атомарная переменная для потокобезопасности

SOCKET ConnectToServer() {
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct sockaddr_in serverAddr;

    // Инициализация Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return INVALID_SOCKET;
    }

    // Создание сокета
    ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ConnectSocket == INVALID_SOCKET) {
        std::cerr << "socket failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return INVALID_SOCKET;
    }

    // Настройка адреса
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    serverAddr.sin_port = htons(SERVER_PORT);

    // Подключение к серверу
    if (connect(ConnectSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "connect failed: " << WSAGetLastError() << std::endl;
        closesocket(ConnectSocket);
        WSACleanup();
        return INVALID_SOCKET;
    }

    return ConnectSocket;
}

DWORD WINAPI ServerListener(LPVOID lpParam) {
    SOCKET ConnectSocket = (SOCKET)lpParam;
    char recvbuf[DEFAULT_BUFLEN];
    int iResult;

    while (Running) {
        iResult = recv(ConnectSocket, recvbuf, DEFAULT_BUFLEN, 0);
        if (iResult > 0) {
            recvbuf[iResult] = '\0';
            if (strcmp(recvbuf, "other_user_press") == 0) {
                std::cout << "Другой пользователь нажал пробел!" << std::endl;
                Beep(1000, 200);
            }
        }
        else if (iResult == 0) {
            std::cout << "Сервер отключился." << std::endl;
            Running = false;
        }
        else {
            std::cerr << "recv failed: " << WSAGetLastError() << std::endl;
            Running = false;
        }
    }
    return 0;
}

int main() {
    SOCKET ConnectSocket = ConnectToServer();
    if (ConnectSocket == INVALID_SOCKET) {
        return 1;
    }

    std::cout << "Подключено к серверу. Нажимайте ПРОБЕЛ для отправки сигнала, Q для выхода." << std::endl;

    // Поток для приема сообщений
    HANDLE hThread = CreateThread(NULL, 0, ServerListener, (LPVOID)ConnectSocket, 0, NULL);
    if (hThread == NULL) {
        std::cerr << "Ошибка создания потока: " << GetLastError() << std::endl;
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    // Основной цикл
    while (Running) {
        if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
            auto start = std::chrono::steady_clock::now();
            Beep(500, 100);

            // Ждем отпускания клавиши
            while (GetAsyncKeyState(VK_SPACE) & 0x8000) {
                Sleep(10);
            }

            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start).count();

            std::cout << "Длительность нажатия: " << duration << " мс" << std::endl;

            // Отправка на сервер с проверкой ошибок
            std::string msg = "press_duration:" + std::to_string(duration);
            if (send(ConnectSocket, msg.c_str(), msg.size(), 0) == SOCKET_ERROR) {
                std::cerr << "send failed: " << WSAGetLastError() << std::endl;
                Running = false;
            }
        }

        if (GetAsyncKeyState('Q') & 0x8000) {
            std::cout << "Завершение работы..." << std::endl;
            Running = false;
        }

        Sleep(10);
    }

    // Корректное завершение
    shutdown(ConnectSocket, SD_BOTH);  // Graceful shutdown
    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    closesocket(ConnectSocket);
    WSACleanup();

    return 0;
}



/* ********************************** ОРИГИНАЛЬНЫЙ КОД ********************************** */
// #include <winsock2.h>
// #include <windows.h>
// #include <iostream>
// #include <chrono>
// #pragma comment(lib, "ws2_32.lib")

// #define SERVER_IP "xxx.yyy.zzz.www" // замените на свой
// #define SERVER_PORT 1234 // замените на свой
// #define DEFAULT_BUFLEN 512

// bool Running = true;

// SOCKET ConnectToServer() {
//     WSADATA wsaData;
//     SOCKET ConnectSocket = INVALID_SOCKET;
//     struct sockaddr_in serverAddr;

//     // Инициализация Winsock
//     int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
//     if (iResult != 0) {
//         std::cerr << "WSAStartup failed: " << iResult << std::endl;
//         return INVALID_SOCKET;
//     }

//     // сокет
//     ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
//     if (ConnectSocket == INVALID_SOCKET) {
//         std::cerr << "socket failed: " << WSAGetLastError() << std::endl;
//         WSACleanup();
//         return INVALID_SOCKET;
//     }

//     // настройка адреса 
//     serverAddr.sin_family = AF_INET;
//     serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
//     serverAddr.sin_port = htons(SERVER_PORT);

//     // подключение
//     iResult = connect(ConnectSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
//     if (iResult == SOCKET_ERROR) {
//         std::cerr << "connect failed: " << WSAGetLastError() << std::endl;
//         closesocket(ConnectSocket);
//         WSACleanup();
//         return INVALID_SOCKET;
//     }

//     return ConnectSocket;
// }

// DWORD WINAPI ServerListener(LPVOID lpParam) {
//     SOCKET ConnectSocket = (SOCKET)lpParam;
//     char recvbuf[DEFAULT_BUFLEN];
//     int iResult;

//     while (Running) {
//         iResult = recv(ConnectSocket, recvbuf, DEFAULT_BUFLEN, 0);
//         if (iResult > 0) {
//             recvbuf[iResult] = '\0';
//             if (strcmp(recvbuf, "other_user_press") == 0) {
//                 std::cout << "Бип" << std::endl;
//                 Beep(1000, 200); // что-то вроде "опа, человек что-то нам говорит"
//             }
//         } else if (iResult == 0) {
//             std::cout << "Сервер отключился." << std::endl; // если сервер корректно вырубился
//             Running = false;
//         } else {
//             std::cerr << "recv failed: " << WSAGetLastError() << std::endl;
//             Running = false;
//         }
//     }
//     return 0;
// }

// int main() {
//     SOCKET ConnectSocket = ConnectToServer();
//     if (ConnectSocket == INVALID_SOCKET) {
//         return 1;
//     }

//     std::cout << "Подключено к серверу. Нажимайте ПРОБЕЛ для отправки сигнала, Q для выхода." << std::endl;

//     // Поток для приема сообщений
//     HANDLE hThread = CreateThread(NULL, 0, ServerListener, (LPVOID)ConnectSocket, 0, NULL);
//     if (hThread == NULL) {
//         std::cerr << "Ошибка создания потока: " << GetLastError() << std::endl;
//         closesocket(ConnectSocket);
//         WSACleanup();
//         return 1;
//     }

//     // Основной цикл
//     while (Running) {
//         if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
//             auto start = std::chrono::steady_clock::now();
//             Beep(500, 100); // Сигнал при нажатии пробела

//             // Ждем отпускания клавиши
//             while (GetAsyncKeyState(VK_SPACE) & 0x8000) {
//                 Sleep(10);
//             }

//             auto end = std::chrono::steady_clock::now();
//             auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

//             std::cout << "Длительность нажатия: " << duration << " мс" << std::endl;

//             // Отправка серверу
//             std::string msg = "press_duration:" + std::to_string(duration);
//             send(ConnectSocket, msg.c_str(), msg.size(), 0);
//         }

//         if (GetAsyncKeyState('Q') & 0x8000) {
//             Running = false;
//         }

//         Sleep(10); // Чтобы не нагружать CPU
//     }

//     // Завершение работы
//     WaitForSingleObject(hThread, INFINITE);
//     CloseHandle(hThread);
//     closesocket(ConnectSocket);
//     WSACleanup();

//     return 0;
// }
