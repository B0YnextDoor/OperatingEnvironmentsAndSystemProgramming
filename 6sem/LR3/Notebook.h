#ifndef NOTEBOOK_H
#define NOTEBOOK_H

#include <string>
#include <vector>

struct Record {
    int id;
    std::string name;
    std::string email;
    std::string phone;
};

class Notebook {
public:
    explicit Notebook(const std::string& filename);

    void addRecord(const std::string& name, const std::string& email, const std::string& phone);
    void displayRecords() const;
    void deleteRecord(int id);
    void searchRecords(const std::string& field, const std::string& value) const;
    void updateRecord(int id, const std::string& name, const std::string& email, const std::string& phone);

private:
    std::string filename;
    int getNextId() const;
    std::vector<Record> loadRecords() const;
    void saveRecords(const std::vector<Record>& records) const;
};

#endif