#include "Notebook.h"
#include <iostream>
#include <string>

void printUsage() {
    std::cout << "Usage:\n"
              << "  add <name> <email> <phone>         — Add record\n"
              << "  delete <id>                       — Delete record by ID\n"
              << "  search <field> <value>            — Find all records by field\n"
              << "  display                           — Show all records\n"
              << "  update <id> <name> <email> <phone> — Update record\n";
}

int main(int argc, char* argv[]) {
    Notebook notebook("notebook.txt");

    if (argc < 2) {
        printUsage();
        return 1;
    }

    std::string command = argv[1];

    if (command == "add") {
        if (argc != 5) {
            std::cerr << "Ошибка: недостаточно аргументов для команды add\n";
            printUsage();
            return 1;
        }
        notebook.addRecord(argv[2], argv[3], argv[4]);
    } else if (command == "delete") {
        if (argc != 3) {
            std::cerr << "Ошибка: недостаточно аргументов для команды delete\n";
            printUsage();
            return 1;
        }
        notebook.deleteRecord(std::stoi(argv[2]));
    } else if (command == "search") {
        if (argc != 4) {
            std::cerr << "Ошибка: недостаточно аргументов для команды search\n";
            printUsage();
            return 1;
        }
        notebook.searchRecords(argv[2], argv[3]);
    } else if (command == "display") {
        notebook.displayRecords();
    } else if (command == "update") {
        if (argc != 6) {
            std::cerr << "Ошибка: недостаточно аргументов для команды update\n";
            printUsage();
            return 1;
        }
        notebook.updateRecord(std::stoi(argv[2]), argv[3], argv[4], argv[5]);
    } else {
        std::cerr << "Неизвестная команда: " << command << "\n";
        printUsage();
        return 1;
    }

    return 0;
}