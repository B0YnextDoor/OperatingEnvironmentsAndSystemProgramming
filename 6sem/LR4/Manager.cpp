#include "Manager.h"
#include <iostream>
#include <thread>
#include <random>

Manager::Manager(int rowsA, int colsA, int colsB, int numAgents)
    : rowsA(rowsA), colsA(colsA), colsB(colsB), numAgents(numAgents) {}

void Manager::generateMatrices() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution dist(1, 10);

    matrixA.resize(rowsA, std::vector<int>(colsA));
    matrixB.resize(colsA, std::vector<int>(colsB));
    result.resize(rowsA, std::vector<int>(colsB, 0));

    for (int i = 0; i < rowsA; ++i) {
        for (int j = 0; j < colsA; ++j) {
            matrixA[i][j] = dist(gen);
        }
    }

    for (int i = 0; i < colsA; ++i) {
        for (int j = 0; j < colsB; ++j) {
            matrixB[i][j] = dist(gen);
        }
    }
}

void Manager::run() {
    generateMatrices();
    
    // Вывод исходных матриц
    std::cout << "Матрица A:" << "\n";
    for (const auto& row : matrixA) {
        for (int val : row) {
            std::cout << val << " ";
        }
        std::cout << "\n";
    }

    std::cout << "Матрица B:" << "\n";
    for (const auto& row : matrixB) {
        for (int val : row) {
            std::cout << val << " ";
        }
        std::cout << "\n";
    }

    // Создание разделяемой памяти для результата
    key_t key = ftok(".", 'R');
    int shmid = shmget(key, rowsA * colsB * sizeof(int), IPC_CREAT | 0666);
    int* shared_result = (int*)shmat(shmid, NULL, 0);

    std::vector<pid_t> children;
    
    // Создание процессов
    for (int i = 0; i < rowsA; ++i) {
        pid_t pid = fork();
        if (pid == 0) {
            Agent agent(i + 1);
            std::vector<int> resultRow(colsB);
            agent.processTask(matrixA[i], matrixB, resultRow);
            for (int j = 0; j < colsB; ++j) {
                shared_result[i * colsB + j] = resultRow[j];
            }
            shmdt(shared_result);
            exit(0);
        } else if (pid > 0) {
            children.push_back(pid);
        }
    }

    // Ожидание завершения всех дочерних процессов
    for (pid_t pid : children) {
        int status;
        waitpid(pid, &status, 0);
    }

    // Копирование результата из разделяемой памяти
    result.resize(rowsA, std::vector<int>(colsB));
    for (int i = 0; i < rowsA; ++i) {
        for (int j = 0; j < colsB; ++j) {
            result[i][j] = shared_result[i * colsB + j];
        }
    }

    // Отключение и удаление разделяемой памяти
    shmdt(shared_result);
    shmctl(shmid, IPC_RMID, NULL);

    // Вывод результата
    std::cout << "Результирующая матрица:" << "\n";
    for (const auto& row : result) {
        for (int val : row) {
            std::cout << val << " ";
        }
        std::cout << "\n";
    }
}