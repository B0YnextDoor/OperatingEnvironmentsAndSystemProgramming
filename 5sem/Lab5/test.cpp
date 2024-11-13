#include <iostream>
#include <winsock2.h>
#include <thread>
#include <string>

#pragma comment(lib, "ws2_32.lib")

void clientFunction(const std::string& serverIP, int serverPort, const std::string& message) {
    SOCKET clientSocket;
    struct sockaddr_in server;

    // Инициализация WinSock
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    // Создание сокета
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    server.sin_family = AF_INET;
    server.sin_port = htons(serverPort);
    server.sin_addr.s_addr = inet_addr(serverIP.c_str());

    // Подключение к серверу
    if (connect(clientSocket, (struct sockaddr*)&server, sizeof(server)) < 0) {
        std::cerr << "Connection error for message: " << message << "\n";
        closesocket(clientSocket);
        WSACleanup();
        return;
    }

    // Отправка сообщения на сервер
    send(clientSocket, message.c_str(), message.size(), 0);

    // Получение ответа от сервера
    char buffer[1024];
    int recvSize = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    buffer[recvSize] = '\0';  // Завершение строки
    std::string res = "Response for message '" + message + "':\n" + buffer + "\n";
    std::cout << res;

    closesocket(clientSocket);
    WSACleanup();
}

int main() {
    const std::string serverIP = "127.0.0.1";  // Локальный хост
    int serverPort = 1111;                      // Порт, на котором запущен сервер

    // Создаем несколько потоков, каждый из которых будет клиентом
    std::thread client1(clientFunction, serverIP, serverPort, "Hello from client 1");
    std::thread client2(clientFunction, serverIP, serverPort, "Hello from client 2");
    std::thread client3(clientFunction, serverIP, serverPort, "Hello from client 3");

    // Ожидаем завершения потоков
    client1.join();
    client2.join();
    client3.join();

    return 0;
}
