#include <vector>

class Agent {
public:
    Agent(int id);
    void processTask(const std::vector<int>& row, const std::vector<std::vector<int>>& matrixB, 
                    std::vector<int>& resultRow);
private:
    int agentId;
};