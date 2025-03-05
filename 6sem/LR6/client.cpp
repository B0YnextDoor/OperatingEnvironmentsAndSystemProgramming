#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <fstream>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    // Создание сокета
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error" << "\n";
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Преобразование IP-адреса
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address/ Address not supported" << "\n";
        return -1;
    }

    // Подключение к серверу
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection failed" << "\n";
        return -1;
    }

    std::cout << "Connected to the server." << "\n";

    // Чтение команд из файла
    std::ifstream script("commands.txt");
    if (!script.is_open()) {
        std::cerr << "Failed to open commands file." << std::endl;
        return -1;
    }

    std::string command;
    while (std::getline(script, command)) {
        command += "\n";
        send(sock, command.c_str(), command.size(), 0);

        // Получение результата
        memset(buffer, 0, BUFFER_SIZE);
        int valread = read(sock, buffer, BUFFER_SIZE);
        if (valread > 0) {
            std::cout << "Result: " << buffer << std::endl;
        } else {
            std::cout << "Server disconnected." << std::endl;
            break;
        }
    }

    script.close();

    // Завершение работы
    send(sock, "exit\n", 5, 0);
    close(sock);

    return 0;
}