#include <iostream>
#include <winsock2.h>
#include <string>

#pragma comment(lib, "ws2_32.lib")

class Client {
public:
    Client(const std::string& serverIP, int serverPort)
        : serverIP(serverIP), serverPort(serverPort), clientSocket(INVALID_SOCKET) {}

    // Инициализация и подключение к серверу
    bool connectToServer() {
        return initializeWinSock() && createSocket() && connectSocket();
    }

    // Завершение работы клиента
    void shutdown() {
        closesocket(clientSocket);
        WSACleanup();
        std::cout << "Client shutdown.\n";
    }

    // Ввод данных пользователя, отправка на сервер и получение ответа
    void communicate() {
		size_t counter = 0;
		std::string userInput;
        while(true) {
			std::cout << "\nRequest #" << counter + 1 << "\n";
        	std::cout << "Enter request to the server (or 'exit' to quit): ";
			std::getline(std::cin, userInput);

			if (userInput == "exit") break; 

			if (sendData(userInput)) {
				std::string response = receiveData();
				if (!response.empty()) {
					std::cout << "Server response: " << response << "\n";
				}
			}

			++counter;
		}
    }

    std::string sendRequest(const std::string& data) {
        std::string error = "";
        if (send(clientSocket, data.c_str(), data.size(), 0) == SOCKET_ERROR) {
			int errorCode = WSAGetLastError();
            error = "Error during sending request. Code: " + std::to_string(errorCode) + " (" + 
            getErrorMessage(errorCode) + ")";
        }
        return error;
    }

    std::string getResponse() {
        char buffer[4096];
        int recvSize = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (recvSize == SOCKET_ERROR) {
			int errorCode = WSAGetLastError();
            return "Server response error. Code: " + std::to_string(errorCode) + " ("+getErrorMessage(errorCode) + ")";
        }
        buffer[recvSize] = '\0';
        return std::string(buffer);
    }

private:
    std::string serverIP;
    int serverPort;
    SOCKET clientSocket;

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
        clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Creating socker error. Code: " << WSAGetLastError() << "\n";
            WSACleanup();
            return false;
        }
        return true;
    }

    // Получение сообщения об ошибке
    std::string getErrorMessage(int errorCode) {
        switch (errorCode) {
            case WSAETIMEDOUT: return "Connection timed out.";
            case WSAECONNREFUSED: return "Connection refused.";
            case WSAENETUNREACH: return "Network unreachable.";
            case WSAEADDRINUSE: return "Address already in use.";
            default: return "An unknown error occurred.";
        }
    }

    // Подключение к серверу
    bool connectSocket() {
        struct sockaddr_in server;
        server.sin_family = AF_INET;
        server.sin_port = htons(serverPort);
        server.sin_addr.s_addr = inet_addr(serverIP.c_str());
		std::cout << "Connecting to " << serverIP << " on port " << serverPort << "...\n";
        if (connect(clientSocket, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
            int errorCode = WSAGetLastError();
            std::cerr << "Can't connect to the server. Code: " << errorCode << " (" << getErrorMessage(errorCode) << ")\n";
            closesocket(clientSocket);
            WSACleanup();
            return false;
        }

        std::cout << "Connected to the server: " << serverIP << " on the port: " << serverPort << ".\n";
        return true;
    }

    // Отправка данных на сервер
    bool sendData(const std::string& data) {
        if (send(clientSocket, data.c_str(), data.size(), 0) == SOCKET_ERROR) {
			int errorCode = WSAGetLastError();
            std::cerr << "Error during sending request. Code: " << errorCode << " (" << getErrorMessage(errorCode) << ")\n";
            return false;
        }
        return true;
    }

    // Получение данных от сервера
    std::string receiveData() {
        char buffer[1024];
        int recvSize = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (recvSize == SOCKET_ERROR) {
			int errorCode = WSAGetLastError();
            std::cerr << "Server response error. Code: " << WSAGetLastError() << " (" << getErrorMessage(errorCode) << ")\n";
            return "";
        }
        buffer[recvSize] = '\0';
        return std::string(buffer);
    }
};

// int main() {
//     std::string serverIP = "127.0.0.1";
//     int serverPort;

//     // Запрос IP и порта сервера у пользователя
//     std::cout << "Enter server port: ";
//     std::cin >> serverPort;
//     std::cin.ignore();  // Игнорируем символ новой строки после порта

//     // Создание и подключение клиента к серверу
//     Client client(serverIP, serverPort);
//     if (client.connectToServer()) {
//         client.communicate();  // Ввод и отправка данных
//     }

//     // Завершение работы клиента
//     client.shutdown();
//     return 0;
// }

//g++ -o client.exe client.cpp -lws2_32
