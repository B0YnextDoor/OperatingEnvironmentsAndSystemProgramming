#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <shared_mutex>
#include <semaphore.h>
#include <atomic>
#include <chrono>
#include <pthread.h>
#include <future>
#include <cmath>
#include <sched.h>
#include <sys/resource.h>
#include <numeric>
#include <fstream>

// Конфигурация тестирования
struct TestConfig {
    int num_threads;
    int num_iterations;
    int work_load;
    int sync_frequency;
    int num_runs;

    TestConfig(int threads = 4, int iters = 10000, int load = 1000, 
              int freq = 10, int runs = 5)
        : num_threads(threads), num_iterations(iters), 
          work_load(load), sync_frequency(freq), num_runs(runs) {}
};

// Результаты тестирования
struct TestResults {
    double mean_time;
    double std_deviation;
    int successful_operations;
    int contention_count;
    
    TestResults() : mean_time(0), std_deviation(0), 
                   successful_operations(0), contention_count(0) {}
};

enum class AccessPattern {
    READ_HEAVY,
    WRITE_HEAVY,
    MIXED
};

// Глобальные переменные для синхронизации
std::mutex mtx;
std::shared_mutex shared_mtx;
sem_t semaphore;
std::atomic<int> counter{0};
pthread_rwlock_t rwlock;
int shared_data = 0;
std::atomic<int> contention_counter{0};

// Класс для сбора статистики
class StatisticsCollector {
private:
    std::vector<double> measurements;
    
public:
    void add_measurement(double time) {
        measurements.push_back(time);
    }
    
    TestResults get_results() const {
        TestResults results;
        if (measurements.empty()) return results;
        
        // Вычисление среднего времени
        results.mean_time = std::accumulate(measurements.begin(), 
                                          measurements.end(), 0.0) / measurements.size();
        
        // Вычисление стандартного отклонения
        double sq_sum = std::inner_product(measurements.begin(), measurements.end(), 
                                         measurements.begin(), 0.0);
        results.std_deviation = std::sqrt(sq_sum / measurements.size() - 
                                        results.mean_time * results.mean_time);
        
        return results;
    }
};

// Имитация вычислительной нагрузки
void simulate_work(int work_load) {
    volatile int dummy = 0;
    for(int i = 0; i < work_load; ++i) {
        dummy += i;
    }
}

// Подготовка к бенчмарку
void prepare_benchmark() {
    // Установка высокого приоритета
    setpriority(PRIO_PROCESS, 0, -20);
    
    // Привязка к CPU
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(0, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
}

// Функции работы с различными механизмами синхронизации
void work_with_mutex(const TestConfig& config, AccessPattern pattern) {
    for (int i = 0; i < config.num_iterations; ++i) {
        simulate_work(config.work_load);
        
        if (i % config.sync_frequency == 0) {
            bool should_write = true;
            switch(pattern) {
                case AccessPattern::READ_HEAVY:
                    should_write = (rand() % 100 < 10);
                    break;
                case AccessPattern::WRITE_HEAVY:
                    should_write = (rand() % 100 < 90);
                    break;
                case AccessPattern::MIXED:
                    should_write = (rand() % 100 < 50);
                    break;
            }
            
            if (should_write) {
                std::lock_guard<std::mutex> lock(mtx);
                ++shared_data;
            } else {
                std::lock_guard<std::mutex> lock(mtx);
                volatile int temp = shared_data;
            }
        }
    }
}

// Работа с shared_mutex
void work_with_shared_mutex(const TestConfig& config, AccessPattern pattern) {
    for (int i = 0; i < config.num_iterations; ++i) {
        simulate_work(config.work_load);
        
        if (i % config.sync_frequency == 0) {
            bool should_write = true;
            switch(pattern) {
                case AccessPattern::READ_HEAVY:
                    should_write = (rand() % 100 < 10);
                    break;
                case AccessPattern::WRITE_HEAVY:
                    should_write = (rand() % 100 < 90);
                    break;
                case AccessPattern::MIXED:
                    should_write = (rand() % 100 < 50);
                    break;
            }
            
            if (should_write) {
                std::unique_lock<std::shared_mutex> lock(shared_mtx);
                ++shared_data;
            } else {
                std::shared_lock<std::shared_mutex> lock(shared_mtx);
                volatile int temp = shared_data;
            }
        }
    }
}

// Работа с семафором
void work_with_semaphore(const TestConfig& config, AccessPattern pattern) {
    for (int i = 0; i < config.num_iterations; ++i) {
        simulate_work(config.work_load);
        
        if (i % config.sync_frequency == 0) {
            bool should_write = true;
            switch(pattern) {
                case AccessPattern::READ_HEAVY:
                    should_write = (rand() % 100 < 10);
                    break;
                case AccessPattern::WRITE_HEAVY:
                    should_write = (rand() % 100 < 90);
                    break;
                case AccessPattern::MIXED:
                    should_write = (rand() % 100 < 50);
                    break;
            }
            
            sem_wait(&semaphore);
            if (should_write) {
                ++shared_data;
            } else {
                volatile int temp = shared_data;
            }
            sem_post(&semaphore);
        }
    }
}

// Работа с атомарными операциями
void work_with_atomic(const TestConfig& config, AccessPattern pattern) {
    for (int i = 0; i < config.num_iterations; ++i) {
        simulate_work(config.work_load);
        
        if (i % config.sync_frequency == 0) {
            bool should_write = true;
            switch(pattern) {
                case AccessPattern::READ_HEAVY:
                    should_write = (rand() % 100 < 10);
                    break;
                case AccessPattern::WRITE_HEAVY:
                    should_write = (rand() % 100 < 90);
                    break;
                case AccessPattern::MIXED:
                    should_write = (rand() % 100 < 50);
                    break;
            }
            
            if (should_write) {
                counter.fetch_add(1, std::memory_order_relaxed);
            } else {
                volatile int temp = counter.load(std::memory_order_relaxed);
            }
        }
    }
}

// Работа с pthread_rwlock
void work_with_rwlock(const TestConfig& config, AccessPattern pattern) {
    for (int i = 0; i < config.num_iterations; ++i) {
        simulate_work(config.work_load);
        
        if (i % config.sync_frequency == 0) {
            bool should_write = true;
            switch(pattern) {
                case AccessPattern::READ_HEAVY:
                    should_write = (rand() % 100 < 10);
                    break;
                case AccessPattern::WRITE_HEAVY:
                    should_write = (rand() % 100 < 90);
                    break;
                case AccessPattern::MIXED:
                    should_write = (rand() % 100 < 50);
                    break;
            }
            
            if (should_write) {
                pthread_rwlock_wrlock(&rwlock);
                ++shared_data;
                pthread_rwlock_unlock(&rwlock);
            } else {
                pthread_rwlock_rdlock(&rwlock);
                volatile int temp = shared_data;
                pthread_rwlock_unlock(&rwlock);
            }
        }
    }
}

// Функция измерения времени
template <typename Function>
TestResults measure_time(const std::string& name, Function func, 
                        const TestConfig& config) {
    StatisticsCollector stats;
    
    for (int run = 0; run < config.num_runs; ++run) {
        shared_data = 0;
        counter = 0;
        contention_counter = 0;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::thread> threads;
        for (int i = 0; i < config.num_threads; ++i) {
            threads.emplace_back(func, std::ref(config), AccessPattern::MIXED);
        }
        
        for (auto& t : threads) {
            t.join();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        
        stats.add_measurement(elapsed.count());
    }
    
    TestResults results = stats.get_results();
    
    // Вывод результатов
    std::cout << "\nResults for " << name << ":\n"
              << "Average time: " << results.mean_time << " seconds\n"
              << "Standard deviation: " << results.std_deviation << " seconds\n"
              << "Contention count: " << contention_counter << "\n"
              << "Final shared_data: " << shared_data << "\n\n";
              
    return results;
}

// Запись результатов в файл
void log_results(const std::string& mechanism, const TestResults& results, 
                const TestConfig& config) {
    std::ofstream log_file("benchmark_results.csv", std::ios::app);
    log_file << mechanism << ","
             << config.num_threads << ","
             << config.num_iterations << ","
             << config.work_load << ","
             << config.sync_frequency << ","
             << results.mean_time << ","
             << results.std_deviation << ","
             << results.successful_operations << ","
             << results.contention_count << "\n";
}

int main() {
    prepare_benchmark();
    
    // Создание заголовка файла с результатами
    std::ofstream log_file("benchmark_results.csv");
    log_file << "Mechanism,Threads,Iterations,WorkLoad,SyncFrequency,MeanTime,StdDev,SuccessOps,Contentions\n";
    log_file.close();
    
    std::vector<TestConfig> configs = {
        TestConfig(2, 10000, 1000, 10, 5),
        TestConfig(4, 10000, 1000, 10, 5),
        TestConfig(8, 10000, 1000, 10, 5)
    };
    
    // Инициализация синхронизационных примитивов
    sem_init(&semaphore, 0, 1);
    pthread_rwlock_init(&rwlock, nullptr);
    
    for (const auto& config : configs) {
        std::cout << "\nTesting with " << config.num_threads << " threads:\n";
        
        // Тестирование всех механизмов
        auto mutex_results = measure_time("Mutex", work_with_mutex, config);
        log_results("mutex", mutex_results, config);
        
        auto shared_mutex_results = measure_time("Shared Mutex", work_with_shared_mutex, config);
        log_results("shared_mutex", shared_mutex_results, config);
        
        auto semaphore_results = measure_time("Semaphore", work_with_semaphore, config);
        log_results("semaphore", semaphore_results, config);
        
        auto atomic_results = measure_time("Atomic", work_with_atomic, config);
        log_results("atomic", atomic_results, config);
        
        auto rwlock_results = measure_time("RWLock", work_with_rwlock, config);
        log_results("rwlock", rwlock_results, config);
        
        std::cout << "\n----------------------------------------\n";
    }
    
    // Очистка
    sem_destroy(&semaphore);
    pthread_rwlock_destroy(&rwlock);
    
    return 0;
}