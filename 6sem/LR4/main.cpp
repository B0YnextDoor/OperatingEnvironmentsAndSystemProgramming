#include "Manager.h"

int main() {
    const int rowsA = 4;
    const int colsA = 3;
    const int colsB = 4;
    const int numAgents = 4;

    Manager manager(rowsA, colsA, colsB, numAgents);
    manager.run();

    return 0;
}