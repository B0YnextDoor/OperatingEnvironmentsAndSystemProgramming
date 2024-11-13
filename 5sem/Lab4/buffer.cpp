#include <windows.h>
#include <iostream>
#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>
#include <atomic>

// Структура заявки
struct Request {
    int id;
    double creation_time;
};

// Структура статистики
struct Statistics {
    int processed_requests = 0;
    int lost_requests = 0;
    double total_processing_time = 0;
    int total_requests = 0;
};

Statistics stats;
// Глобальные переменные для отображения состояния
std::vector<int> channel_states;
std::mutex state_mutex;
HWND hListBox; // Переменная для ListBox

// Буфер (очередь) для заявок с синхронизацией
class Buffer {
private:
    std::queue<Request> requests;
    std::mutex mtx;
    std::condition_variable cv;
    size_t capacity;

public:
    Buffer(size_t capacity) : capacity(capacity) {}

    bool push(const Request& req) {
        std::unique_lock<std::mutex> lock(mtx);
        if (requests.size() >= capacity) {
            return false; // буфер полон
        }
        requests.push(req);
        cv.notify_one();
        return true;
    }

    Request pop() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]() { return !requests.empty(); });
        Request req = requests.front();
        requests.pop();
        return req;
    }

    size_t size() {
        std::lock_guard<std::mutex> lock(mtx);
        return requests.size();
    }
    
    bool is_full() {
        std::lock_guard<std::mutex> lock(mtx);
        return requests.size() >= capacity;
    }
};

// Барьер для синхронизации всех каналов ступени
class Barrier {
private:
    std::mutex mtx;
    std::condition_variable cv;
    int count;
    int threshold;

public:
    Barrier(int threshold) : threshold(threshold), count(0) {}

    void wait() {
        std::unique_lock<std::mutex> lock(mtx);
        count++;
        if (count == threshold) {
            count = 0; // Сбрасываем счетчик
            cv.notify_all(); // Все каналы достигли барьера
        } else {
            cv.wait(lock, [this]() { return count == 0; });
        }
    }
};

// Функция для работы потока (канала)
void channel(Buffer& input_buffer, Buffer* output_buffer, int id, int processing_time, Barrier& barrier, std::atomic<bool>& stage_active) {
    while (stage_active) {
        // Ожидание заявок
        if (input_buffer.size() > 0) {
            Request req = input_buffer.pop();
            // Обновляем состояние канала для отображения
            {
                std::lock_guard<std::mutex> lock(state_mutex);
                channel_states[id] = 1; // Канал занят
            }
            std::string buf = "Channel " + std::to_string(id + 1) + " is processing the request " + std::to_string(req.id) + "\n";
            SendMessageA(hListBox, LB_ADDSTRING, 0, (LPARAM)buf.c_str());
            std::cout << buf;

            // Имитация обработки
            std::this_thread::sleep_for(std::chrono::milliseconds(processing_time));

            // Обновляем состояние канала для отображения
            {
                std::lock_guard<std::mutex> lock(state_mutex);
                channel_states[id] = 0; // Канал свободен
            }

            buf = "Channel " + std::to_string(id + 1) + " has finished processing the request " + std::to_string(req.id) + "\n";
            SendMessageA(hListBox, LB_ADDSTRING, 0, (LPARAM)buf.c_str());
            std::cout << buf;

            // Если есть выходной буфер, передаем заявку в следующий буфер (на следующую ступень)
            if (output_buffer) {
                if (!output_buffer->push(req)) {
                    buf = "Request " + std::to_string(req.id) + " was lost (next buffer full)\n";
                    stats.lost_requests++; // Штрафы за необработанные заявки
                    SendMessageA(hListBox, LB_ADDSTRING, 0, (LPARAM)buf.c_str());
                    std::cout << buf;
                } else {
                    buf = "Request " + std::to_string(req.id) + " moved to the next stage\n";
                    SendMessageA(hListBox, LB_ADDSTRING, 0, (LPARAM)buf.c_str());
                    std::cout << buf;
                }
            } else {
                stats.processed_requests++;
                stats.total_processing_time += processing_time;
            }

            // Барьерная синхронизация: ждем завершения всех каналов
            barrier.wait();
        }
    }
}