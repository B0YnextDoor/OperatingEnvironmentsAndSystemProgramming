#include "Agent.h"
#include <iostream>

Agent::Agent(int id) : agentId(id) {}

void Agent::processTask(const std::vector<int>& row, const std::vector<std::vector<int>>& matrixB, 
                        std::vector<int>& resultRow) {
    int numColsB = matrixB[0].size();
    for (int col = 0; col < numColsB; ++col) {
        int sum = 0;
        for (size_t i = 0; i < row.size(); ++i) {
            sum += row[i] * matrixB[i][col];
        }
        resultRow[col] = sum;
    }
    std::cout << "Агент " << agentId << " обработал строку." << "\n";
}