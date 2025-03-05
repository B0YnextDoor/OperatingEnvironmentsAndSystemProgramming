#include "Notebook.h"
#include <fstream>
#include <iostream>
#include <sstream>

Notebook::Notebook(const std::string& filename) : filename(filename) {}

int Notebook::getNextId() const {
    std::vector<Record> records = loadRecords();
    int maxId = 0;
    for (const auto& record : records) {
        if (record.id > maxId) {
            maxId = record.id;
        }
    }
    return maxId + 1;
}

std::vector<Record> Notebook::loadRecords() const {
    std::vector<Record> records;
    std::ifstream file(filename);
    if (!file.is_open()) {
        return records;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream stream(line);
        Record record;
        std::getline(stream, line, '|');
        record.id = std::stoi(line);
        std::getline(stream, record.name, '|');
        std::getline(stream, record.email, '|');
        std::getline(stream, record.phone, '|');
        records.push_back(record);
    }

    return records;
}

void Notebook::saveRecords(const std::vector<Record>& records) const {
    std::ofstream file(filename, std::ios::trunc);
    if (!file.is_open()) {
        std::cerr << "Error! Can't open file: " << filename << "\n";
        return;
    }

    for (const auto& record : records) {
        file << record.id << "|" << record.name << "|" << record.email << "|" << record.phone << "\n";
    }
}

void Notebook::addRecord(const std::string& name, const std::string& email, const std::string& phone) {
    int id = getNextId();
    std::vector<Record> records = loadRecords();

    records.push_back({id, name, email, phone});
    saveRecords(records);

    std::cout << "Record added: ID=" << id << ", Name=" << name << ", Email=" << email << ", Phone=" << phone << "\n";
}

void Notebook::displayRecords() const {
    std::vector<Record> records = loadRecords();
    if (records.empty()) {
        std::cout << "Notebook is empty." << "\n";
        return;
    }

    for (const auto& record : records) {
        std::cout << "ID: " << record.id << ", Name: " << record.name << ", Email: " << record.email << ", Phone: " << record.phone << "\n";
    }
}

void Notebook::deleteRecord(int id) {
    std::vector<Record> records = loadRecords();
    bool found = false;

    std::vector<Record> updatedRecords;
    for (const auto& record : records) {
        if (record.id == id) {
            found = true; // Запись найдена
        } else {
            updatedRecords.push_back(record); // Сохраняем остальные записи
        }
    }

    if (found) {
        saveRecords(updatedRecords);
        std::cout << "Record with ID " << id << " deleted." << "\n";
    } else {
        std::cout << "Record with ID " << id << " not found." << "\n";
    }
}

void Notebook::searchRecords(const std::string& field, const std::string& value) const {
    std::vector<Record> records = loadRecords();
    bool found = false;

    for (const auto& record : records) {
        if ((field == "name" && record.name == value) ||
            (field == "email" && record.email == value) ||
            (field == "phone" && record.phone == value)) {
            std::cout << "ID: " << record.id << ", Name: " << record.name << ", Email: " << record.email << ", Phone: " << record.phone << "\n";
            found = true;
        }
    }

    if (!found) {
        std::cout << "Records with " << field << " = " << value << " not found." << "\n";
    }
}

void Notebook::updateRecord(int id, const std::string& name, const std::string& email, const std::string& phone) {
    std::vector<Record> records = loadRecords();
    bool found = false;

    for (auto& record : records) {
        if (record.id == id) {
            record.name = name;
            record.email = email;
            record.phone = phone;
            found = true;
            break;
        }
    }

    if (found) {
        saveRecords(records);
        std::cout << "Record with ID " << id << " updated." << "\n";
    } else {
        std::cout << "Record with ID " << id << " not found." << "\n";
    }
}