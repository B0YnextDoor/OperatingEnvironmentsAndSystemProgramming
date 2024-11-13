#include <iostream>
#include <winsock2.h>
#include <thread>
#include <map>
#include <string>
#include <sstream>

#pragma comment(lib, "ws2_32.lib")

// Таблицы для преобразования в/из азбуки Морзе
std::map<char, std::string> char_to_morse = {
    {'A', ".-"}, {'B', "-..."}, {'C', "-.-."}, {'D', "-.."}, {'E', "."}, {'F', "..-."}, {'G', "--."}, {'H', "...."}, {'I', ".."}, {'J', ".---"}, {'K', "-.-"}, {'L', ".-.."}, {'M', "--"}, {'N', "-."}, {'O', "---"}, {'P', ".--."}, {'Q', "--.-"}, {'R', ".-."}, {'S', "..."}, {'T', "-"}, {'U', "..-"}, {'V', "...-"}, {'W', ".--"}, {'X', "-..-"}, {'Y', "-.--"}, {'Z', "--.."}, {'1', ".----"}, {'2', "..---"}, {'3', "...--"}, {'4', "....-"}, {'5', "....."}, 
    {'6', "-...."}, {'7', "--..."}, {'8', "---.."}, {'9', "----."}, {'0', "-----"}, {',', "--..--"}, {'!', "-.-.--"}, 
    {'?', "..--.."}, {'.', ".-.-.-"}, {'\'', ".----."}, {' ', "......."}
};

std::map<std::string, char> morse_to_char;

// Инициализация обратного словаря
void initializeMorseToChar() {
    for (const auto& pair : char_to_morse) {
        morse_to_char[pair.second] = pair.first;
    }
}

class Server {
public:
    Server(int port) : port(port), serverSocket(INVALID_SOCKET) {}

    // Инициализация и запуск сервера
    bool start() {
        if (initializeWinSock() && createSocket() && bindSocket()) {
			listenForConnections();
        	return true;
		}
		return false;
    }

    // Завершение работы сервера
    void shutdown() {
        closesocket(serverSocket);
        WSACleanup();
        std::cout << "Server shutdown.\n";
    }

private:
    int port;
    SOCKET serverSocket;

    // Инициализация WinSock
    bool initializeWinSock() {
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
            std::cerr << "WinSock initialization error. Code: " << WSAGetLastError() << "\n";
            return false;
        }
        return true;
    }

    // Создание сокета
    bool createSocket() {
        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == INVALID_SOCKET) {
            std::cerr << "Creating socker error. Code: " << WSAGetLastError() << "\n";
            WSACleanup();
            return false;
        }
        return true;
    }

    // Привязка сокета к порту
    bool bindSocket() {
        struct sockaddr_in server;
        server.sin_family = AF_INET;
        server.sin_addr.s_addr = INADDR_ANY;
        server.sin_port = htons(port);

        if (bind(serverSocket, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
            std::cerr << "Can't bind socket to the port " << port << ". Code: " << WSAGetLastError() << "\n";
            closesocket(serverSocket);
            WSACleanup();
            return false;
        }
        return true;
    }

    // Прослушивание и обработка подключений клиентов
    void listenForConnections() {
        listen(serverSocket, 3);
        std::cout << "The server is running and waiting for connections on the port " << port << "...\n";

        struct sockaddr_in client;
        int clientSize = sizeof(client);

        while (true) {
            SOCKET clientSocket = accept(serverSocket, (struct sockaddr*)&client, &clientSize);
            if (clientSocket != INVALID_SOCKET) {
            std::cout << "Client connected!\n";
            std::thread clientThread([this, clientSocket]() {
                    handleClient(clientSocket);
            });
            clientThread.detach();
			} else {
				std::cerr << "Client connection error. Code: " << WSAGetLastError() << "\n";
			}
        }
    }

    // Обработка данных от клиента и отправка ответа
    void handleClient(SOCKET clientSocket) {
		char buffer[1024];
		int recvSize;

		// Цикл обработки запросов
		while ((recvSize = recv(clientSocket, buffer, sizeof(buffer) - 1, 0)) > 0) {
			buffer[recvSize] = '\0';
			std::string inputText(buffer);
			std::string response;
			if (inputText.find(".") == 0 || inputText.find("-") == 0) {
				response = convertFromMorse(inputText);
			} else {
				response = convertToMorse(inputText);
			}
			if (send(clientSocket, response.c_str(), response.size(), 0) == SOCKET_ERROR) {
				std::cerr << "Error during sending response to the client. Code: " << WSAGetLastError() << "\n";
				break;
			}
		}

		if (recvSize == SOCKET_ERROR) {
			std::cerr << "Handling client data error. Code: " << WSAGetLastError() << "\n";
		}

		closesocket(clientSocket);
        std::cout << "Client disconnected!\n";
    }

    // Конвертация строки в азбуку Морзе
    std::string convertToMorse(const std::string& text) {
        std::string result;
        for (char c : text) {
            char upper = toupper(c);
            if (char_to_morse.find(upper) != char_to_morse.end()) {
                result += char_to_morse[upper] + " ";
            } else {
                result += " ";
            }
        }
        return result;
    }

    // Расшифровка из азбуки Морзе в английский текст
    std::string convertFromMorse(const std::string& morse) {
        std::istringstream iss(morse);
        std::string token;
        std::string result;

        while (iss >> token) {
            if (morse_to_char.find(token) != morse_to_char.end()) {
                result += morse_to_char[token];
            } else {
                result += " ";
            }
        }
        return result;
    }
};

int main() {
    initializeMorseToChar();

    int port;
    std::cout << "Enter server port: ";
    std::cin >> port;

    if (port < 1024 || port > 65535) {
        std::cerr << "Error: port must be in the range 1024-65535.\n";
        return 1;
    }

    Server server(port);
    if (server.start()) {
        std::cout << "Server succsessfully started.\n";
    } else {
        std::cerr << "Can't start the server.\n";
    }

    server.shutdown();
    return 0;
}

//g++ -o server.exe server.cpp -lws2_32
