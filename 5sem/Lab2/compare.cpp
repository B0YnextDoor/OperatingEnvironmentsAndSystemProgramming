#include <chrono>
#include "db.cpp"
#include "sync_db.cpp"

struct Record {
    char name[32];
    int age;

	std::string toString() const {
        return " --- Name: " + std::string(name) + " --- Age: " + std::to_string(age);
    }
};

Record rec = {"Test", 123};

template<typename T>
double benchmark(T& db) {
	auto start = std::chrono::high_resolution_clock::now();

	for (size_t i = 0; i < 1000000; ++i) {
        db.addRecord(rec);
    }

	for (size_t i = 0; i < 1000; ++i) {
        db.getRecord(i + 1);
    }

	for (size_t i = 0; i < 1000; ++i) {
        db.deleteRecord(i + 1);
    }

	auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
	return elapsed.count();
}

int main() {
	Database<Record> db1("file1.dat", 1000);
	SyncDatabase<Record> db2("file2.dat");

	double time1, time2;

	time1 = benchmark<Database<Record>>(db1);
	time2 = benchmark<SyncDatabase<Record>>(db2);

	std::cout << "In-Memory-Mapped file: " <<  time1 << "\n";
	std::cout << "Sync file: " << time2 << "\n";
}