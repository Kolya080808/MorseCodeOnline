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

const int sampleRate = 44100;
const int freq = 440;
const int bufferSize = 44100 / 10;
short buffer[bufferSize];

void FillBuffer() {
    for (int i = 0; i < bufferSize; ++i) {
        buffer[i] = 32767 * sin(2 * M_PI * freq * i / sampleRate);
    }
}

void StartTone() {
    WAVEFORMATEX wfx = {0};
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 1;
    wfx.nSamplesPerSec = sampleRate;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    FillBuffer();

    if (waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) return;
}

void StopTone() {
    if (hWaveOut) {
        waveOutReset(hWaveOut);
        waveOutClose(hWaveOut);
        hWaveOut = NULL;
    }
}

DWORD WINAPI ToneThread(LPVOID) {
    StartTone();

    WAVEHDR header[2];
    ZeroMemory(&header, sizeof(header));

    for (int i = 0; i < 2; ++i) {
        header[i].lpData = (LPSTR)buffer;
        header[i].dwBufferLength = sizeof(buffer);
        waveOutPrepareHeader(hWaveOut, &header[i], sizeof(WAVEHDR));
    }

    int index = 0;

    while (Running) {
        if (Beeping) {
            if (!(header[index].dwFlags & WHDR_INQUEUE)) {
                waveOutWrite(hWaveOut, &header[index], sizeof(WAVEHDR));
                index = (index + 1) % 2;
            }
        } else {
            waveOutReset(hWaveOut);
        }
        Sleep(5);
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

DWORD WINAPI PulsingThread(LPVOID lpParam) {
    SOCKET sock = (SOCKET)lpParam;
    bool pipipimode = false;

    while (Running) {
        bool leftPressed = GetAsyncKeyState(VK_LEFT) & 0x8000;
        bool rightPressed = GetAsyncKeyState(VK_RIGHT) & 0x8000;

        if (rightPressed && !leftPressed) {
            pipipimode = true;
            send(sock, "space_down", 11, 0);
            Sleep(15); // можете изменить значение если нужна меньшая длина гудков
            send(sock, "space_up", 9, 0);
        } else {
            pipipimode = false;
        }

        Sleep(pipipimode ? 70 : 10); // можете изменить значение если нужно быстрее (уменьшить чтобы было быстрее ТОЛЬКО ЗНАЧЕНИЕ СПРАВА (70))
    }

    return 0;
}

int main() {
    SOCKET sock = ConnectToServer();
    if (sock == INVALID_SOCKET) return 1;

    HANDLE hToneThread = CreateThread(NULL, 0, ToneThread, NULL, 0, NULL);
    HANDLE hListenThread = CreateThread(NULL, 0, ServerListener, (LPVOID)sock, 0, NULL);
    HANDLE hPulseThread = CreateThread(NULL, 0, PulsingThread, (LPVOID)sock, 0, NULL);

    std::cout << "Use LEFT for long beep, RIGHT for pulsing beep. ESC to exit.\n";

    bool wasLongPressed = false;

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

        bool longPressed = GetAsyncKeyState(VK_LEFT) & 0x8000;
        bool shortPressed = GetAsyncKeyState(VK_RIGHT) & 0x8000;

        // Приоритет длинному сигналу
        if (longPressed && !wasLongPressed) {
            send(sock, "space_down", 11, 0);
        } else if (!longPressed && wasLongPressed) {
            send(sock, "space_up", 9, 0);
        }

        wasLongPressed = longPressed;

        Sleep(1);
    }

    shutdown(sock, SD_BOTH);
    closesocket(sock);
    WSACleanup();

    WaitForSingleObject(hToneThread, INFINITE);
    WaitForSingleObject(hListenThread, INFINITE);
    WaitForSingleObject(hPulseThread, INFINITE);
    CloseHandle(hToneThread);
    CloseHandle(hListenThread);
    CloseHandle(hPulseThread);

    return 0;
}

