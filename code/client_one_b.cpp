#include <winsock2.h>
#include <windows.h>
#include <mmsystem.h>
#include <thread>
#include <atomic>
#include <iostream>
#include <cmath>
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "127.0.0.1" // ПОМЕНЯЙТЕ НА СВОЙ АДРЕС
#define SERVER_PORT 1234 // ПОМЕНЯЙТЕ НА СВОЙ ПОРТ
#define DEFAULT_BUFLEN 512

std::atomic<bool> Running(true);
std::atomic<bool> Beeping(false);
HWAVEOUT hWaveOut = NULL;
WAVEHDR hdr = {0};

void StartTone() {
    const int sampleRate = 44100;
    const int freq = 440;
    static short buffer[44100];

    for (int i = 0; i < sampleRate; ++i) {
        buffer[i] = 32767 * sin(2 * M_PI * freq * i / sampleRate);
    }

    WAVEFORMATEX wfx = {0};
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 1;
    wfx.nSamplesPerSec = sampleRate;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    if (waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) return;

    hdr.lpData = (LPSTR)buffer;
    hdr.dwBufferLength = sizeof(buffer);
    hdr.dwFlags = WHDR_BEGINLOOP | WHDR_ENDLOOP;
    hdr.dwLoops = 0xFFFFFFFF;

    waveOutPrepareHeader(hWaveOut, &hdr, sizeof(WAVEHDR));
    waveOutWrite(hWaveOut, &hdr, sizeof(WAVEHDR));
}

void StopTone() {
    if (hWaveOut) {
        waveOutReset(hWaveOut);
        waveOutUnprepareHeader(hWaveOut, &hdr, sizeof(WAVEHDR));
        waveOutClose(hWaveOut);
        hWaveOut = NULL;
    }
}

SOCKET ConnectToServer() {
    WSADATA wsaData;
    SOCKET ConnectSocket;
    sockaddr_in serverAddr;

    WSAStartup(MAKEWORD(2, 2), &wsaData);
    ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    serverAddr.sin_port = htons(SERVER_PORT);

    connect(ConnectSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    return ConnectSocket;
}

DWORD WINAPI ToneThread(LPVOID) {
    while (Running) {
        if (Beeping && hWaveOut == NULL) {
            StartTone();
        } else if (!Beeping && hWaveOut != NULL) {
            StopTone();
        }
        Sleep(50);
    }
    StopTone();
    return 0;
}

DWORD WINAPI ServerListener(LPVOID lpParam) {
    SOCKET sock = (SOCKET)lpParam;
    char buffer[DEFAULT_BUFLEN];

    while (Running) {
        int recvSize = recv(sock, buffer, DEFAULT_BUFLEN - 1, 0);
        if (recvSize <= 0) break;

        buffer[recvSize] = '\0';
        std::string msg(buffer);

        if (msg == "space_down") Beeping = true;
        else if (msg == "space_up") Beeping = false;
    }
    Running = false;
    return 0;
}

int main() {
    setlocale(LC_ALL, "RU"); // локализировал консоль чтобы не было нечитаемого текста
    SetConsoleOutputCP(CP_UTF8);      // консоль — на UTF-8 (иначе тоже будет нечитаемый текст)
    SetConsoleCP(CP_UTF8);            // ввод — тоже



    SOCKET sock = ConnectToServer();
    if (sock == INVALID_SOCKET) return 1;

    HANDLE hToneThread = CreateThread(NULL, 0, ToneThread, NULL, 0, NULL);
    HANDLE hListenThread = CreateThread(NULL, 0, ServerListener, (LPVOID)sock, 0, NULL);

    std::cout << "Нажмите ПРОБЕЛ для короткого звука. Зажмите для длинного. Нажмите ESC для выхода.\n";

    while (Running) {
        HWND foreground = GetForegroundWindow();
        if (foreground != GetConsoleWindow()) {
            Sleep(50);
            continue;
        }

        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            Running = false;
            break;
        }

        static bool wasPressed = false;
        bool pressed = GetAsyncKeyState(VK_SPACE) & 0x8000;

        if (pressed && !wasPressed) {
            send(sock, "space_down", 11, 0);
        } else if (!pressed && wasPressed) {
            send(sock, "space_up", 9, 0);
        }
        wasPressed = pressed;
        Sleep(1);
    }

    shutdown(sock, SD_BOTH);
    closesocket(sock);
    WSACleanup();

    WaitForSingleObject(hToneThread, INFINITE);
    WaitForSingleObject(hListenThread, INFINITE);
    CloseHandle(hToneThread);
    CloseHandle(hListenThread);

    return 0;
}

