#include "buffer.cpp"

// Ступень с несколькими каналами и барьерной синхронизацией
class Stage {
private:
    std::vector<std::thread> channels;
    Buffer& input_buffer;
    Buffer* output_buffer;  // Выходной буфер для передачи заявок на следующую ступень
    Barrier barrier;
    std::atomic<bool> stage_active;
    int num_channels;
    int processing_time;

public:
    Stage(int num_channels, int processing_time, Buffer& input_buffer, Buffer* output_buffer)
        : input_buffer(input_buffer), output_buffer(output_buffer), barrier(num_channels), stage_active(true), num_channels(num_channels), processing_time(processing_time) {
        for (int i = 0; i < num_channels; i++) {
            int id = i;
            if(!output_buffer) {
                id += num_channels;
            }
            channels.emplace_back(channel, std::ref(input_buffer), output_buffer, id, processing_time, std::ref(barrier), std::ref(stage_active));
        }
    }

    ~Stage() {
        // Останавливаем работу ступени
        stage_active = false;
        for (auto& ch : channels) {
            if (ch.joinable()) {
                // Ждем завершения всех каналов
                ch.join();
            }
        }
    }
};

void request_generator(Buffer& buffer, int intensity, int num_requests) {
    std::string buf;
    for (int i = 0; i < num_requests; i++) {
        Request req = {i, static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::high_resolution_clock::now().time_since_epoch()).count())};
        buf = "Request " + std::to_string(i);
        if (!buffer.push(req)) {
            buf += " is waiting (buffer is full).\n";
            stats.lost_requests++; // Штрафы за необработанные заявки
        } else {
            buf += " is added to buffer.\n";
        }
        std::cout << buf;
        SendMessageA(hListBox, LB_ADDSTRING, 0, (LPARAM)buf.c_str());
        std::this_thread::sleep_for(std::chrono::milliseconds(intensity)); // Интервал между заявками
    }
    
    // Завершение генерации заявок
    buf = "Request generation completed.\n";
    SendMessageA(hListBox, LB_ADDSTRING, 0, (LPARAM)buf.c_str());
    std::cout << buf;
}

