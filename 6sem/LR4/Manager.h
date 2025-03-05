#include <vector>
#include "Agent.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/shm.h>

class Manager {
public:
    Manager(int rowsA, int colsA, int colsB, int numAgents);
    void run();

private:
    void generateMatrices();
    int rowsA, colsA, colsB, numAgents;
    std::vector<std::vector<int>> matrixA;
    std::vector<std::vector<int>> matrixB;
    std::vector<std::vector<int>> result;
};