# MorseCodeOnline
Попытался по такому краткому тех заданию друга (конечно были и дополнения) написать код для сервера и клиента чтобы набирать сообщение кодом морзе когда у тебя просто есть комп на виндовс.

## ДИСКЛЕЙМЕР

Это фигня может плохо работать. Официальный [релиз](https://github.com/Kolya080808/MorseCodeAutoListener/releases/tag/v1.0) есть, но оно может все равно работать плохо, ибо цпп я знаю плохо. Так же учтите, что есть огромная вероятность, что Microsoft Defender будет ругаться, так как экзешник не был подписан или скомпилирован в visual studio. Вирусов в нем нет, код открытый, можете проверить сами.

## Как это работает (по крайней мере как это должно работать)?
Ты запускаешь бинарь. Он инициализирует подключение к серверу и ждет нажатий стрелочек/пробела/получения сообщения/нажатия ESC. Дальше зависит от клиента:

Если вы скачали [client_one_b.cpp/exe](https://github.com/Kolya080808/MorseCodeAutoListener/blob/main/code/client_one_b.cpp), здесь может произойти три вещи:

1. Если ты нажал пробел:
- код отправляет space_down на сервер
- сам получает сообщение от него и пищит пока не получит space_up (пока вы не отпустите пробел)
2. Если ты получил сообщение:
- твоя программа просто издает звуковой сигнал.
3. Если ты нажал ESC:
- ты закроешь программу.


Если вы скачали [client_two_b.cpp/exe](https://github.com/Kolya080808/MorseCodeAutoListener/blob/main/code/client_two_b.cpp), здесь также может произойти три вещи:

1. Если ты нажал на стрелочки:
  Если на правую:
  - Код каждые отправляет на сервер space_down
  - Клиент сам от себя получает space_down и пищит
  - Через x секунд на сервер отправляется space_up
  - Клиент тоже это получает и перестает пищать
  - Через y секунд он повторяет это пока ты не отпустишь кнопку.
  Если на левую:
  - Код каждые отправляет на сервер space_down
  - Клиент сам от себя получает space_down и пищит
  - Через x/3 секунд на сервер отправляется space_up
  - Клиент тоже это получает и перестает пищать
  - Через y секунд он повторяет это пока ты не отпустишь кнопку.
3. Если ты получил сообщение:
  - твоя программа просто издает звуковые сигналы (получает space_up, space_down и т. д.).
3. Если ты нажал ESC:
  - ты закроешь программу.

## Как это использовать?

1. Скачайте код для [сервера](https://github.com/Kolya080808/MorseCodeAutoListener/blob/main/code/server.cpp) и [клиента с одной кнопкой](https://github.com/Kolya080808/MorseCodeAutoListener/blob/main/code/client_one_b.cpp)/[двумя кнопками](https://github.com/Kolya080808/MorseCodeAutoListener/blob/main/code/client_two_b.cpp), так же iso [windows SDK](https://developer.microsoft.com/ru-ru/windows/downloads/windows-sdk/).
2. Поменяйте порт и ip в сервере и клиенте
3. На линуксе (я работаю там) надо сделать так:
```bash
sudo apt update; sudo apt upgrade; sudo apt install g++-mingw-w64-x86-64 -y; x86_64-w64-mingw32-g++ server.cpp -o server.exe -lws2_32 -lwinmm -static; x86_64-w64-mingw32-g++ client_{one/two}_b.cpp -o client.exe -lwinmm -lws2_32 -static; sleep 10; clear; echo "установлено :)"; sleep 10; clear
```
4. Отправьте на сервер экзешник и запустите его
### Далее, если антивирус ругается на client.exe и/или server.exe, нужно подписать скомпилированный экзешник. Сначала перенесем его в папку загрузок.
5. После этого распакуйте windows SDK, который мы скачали раннее, и запустите установшик.
6. Далее выберите папку загрузок и нажмите три раза далее.
7. Затем выберите только установку signing tools, чтобы все выглядело вот так:
![image](https://github.com/user-attachments/assets/865a7e23-26c1-4178-8a76-1480146cb001)
8. Нажмите установить.
9. Запустите powershell от имени администратора и введите команду `New-SelfSignedCertificate -Type CodeSigningCert -Subject "CN=MyTestCertificate" -KeyUsage DigitalSignature -CertStoreLocation "Cert:\CurrentUser\My"`, чтобы создать самописный сертификат.
10. Зайдите в cmd, и напишите `cd "C:\Program Files (x86)\Windows Kits\10\bin\version\x86"`, и потом `signtool sign /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 /a "C:\path\to\file.exe"`. Таким образом вы подпишите исполняемый файл и антивирус ругаться не будет.
11. Залейте на сервер экзешник сервера.
12. Сами запустите экзешник клиента и следите за сервером: получает ли он подключение, записывает ли стрелочку вправо или влево и так далее. Если записывает, то вы должны слышать звуковые сигналы. Иначе вы их слышать не будете.

### Если потом надо удалить windows sdk и сертификат, это делается так:


![image](https://github.com/user-attachments/assets/d1f8ec4f-e9a6-47ed-b80b-0e5afe50eb9f)
![image](https://github.com/user-attachments/assets/8e1ab20f-3745-4fa9-a8a3-5716fffafb99)
![image](https://github.com/user-attachments/assets/937b12b4-9bc3-4e6e-9ec8-b697a4f919b9) - это надо удалить
![image](https://github.com/user-attachments/assets/f11b7e5f-7ad3-481c-9a08-e643d687da61) - это надо удалить

![image](https://github.com/user-attachments/assets/00ce7c9c-51c8-44d1-b5b6-8ffe2be3c964)
![image](https://github.com/user-attachments/assets/e7867cc5-7d9a-4794-80c0-312a49110d69)
```powershell
$cert = Get-ChildItem -Path Cert:\CurrentUser\My | Where-Object { $_.Subject -eq "CN=MyTestCertificate" }
```
```powershell
Remove-Item -Path $cert.PSPath
```
Ну и папку с установщиком.
## ДРУГОЙ СПОСОБ

Скачайте на [странице релизов](https://github.com/Kolya080808/MorseCodeAutoListener/releases/tag/v1.0) готовый екзешник клиента с [одной](https://github.com/Kolya080808/MorseCodeAutoListener/releases/download/v1.0/client_one_b.exe)/[двумя](https://github.com/Kolya080808/MorseCodeAutoListener/releases/download/v1.0/client_two_b.exe) кнопками или [сервера](https://github.com/Kolya080808/MorseCodeAutoListener/releases/download/v1.0/server.exe) или [все вместе](https://github.com/Kolya080808/MorseCodeAutoListener/releases/download/v1.0/executables.zip). Они будут работать только на локальной машине.

# УЧТИТЕ! СЕРВЕР НЕ БУДЕТ ЗАПУЩЕН ЕСЛИ ПОРТ ЗАНЯТ, А ТАК ЖЕ КЛИЕНТ НЕ БУДЕТ ЗАПУЩЕН ЕСЛИ НЕТ ПОДКЛЮЧЕНИЯ К СЕРВЕРУ!

## Как проверить, открыт ли порт?
```win+r```
```cmd```
```powershell
netstat -ano | findstr :<port>
```

# Итог

## Чья идея?

[Kuzya_Kotav](https://github.com/Kuzya-kotav), [mihanN1](https://github.com/MihanN1) - вот их, я просто соучастник, только код делаю.

## Стоит ли этим пользоваться?

Вообще без понятия. Я хоть и знаю азбуку морзе, но не спец в таком :)
Я попытался, мне было нормально как с двумя, так и с одной кнопкой. Так что зависит от вас, я делал под себя.
