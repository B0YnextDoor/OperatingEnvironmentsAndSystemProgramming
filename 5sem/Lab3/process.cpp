#include "mutex.cpp"
#include "shared_mem.cpp"
#include <thread>
#include <vector>
#include <atomic>

HWND hEditOutput; // Глобальный элемент управления для вывода данных
std::vector<std::thread> threads; // Вектор для хранения потоков
std::atomic<bool> stopFlag(false); // Флаг остановки процессов

// Класс, представляющий процесс, работающий с буферами
class UserProcess {
private:
    int id;
    SharedMemory& shared_memory;
    Mutex& mutex;

public:
    UserProcess(int id, SharedMemory& shared_mem, Mutex& mtx) 
        : id(id), shared_memory(shared_mem), mutex(mtx) {}

    // Функция работы с буферами
   void operator()() {
		if(id < 0) return;
		int counter = 0; // Счетчик операций процесса
		stopFlag = false;
		std::string output;
		while (!stopFlag && counter < 10) {
			SharedBuffer* buffer = nullptr;

			// Захват мьютекса для поиска свободного буфера
			mutex.lock();
			for (int i = 0; i < BUFFER_COUNT; ++i) {
				if (!shared_memory.getBuffer(i)->is_used) {
					buffer = shared_memory.getBuffer(i);
					buffer->is_used = true;
					break;
				}
			}
			mutex.unlock();

			int len = GetWindowTextLength(hEditOutput);
			SendMessage(hEditOutput, EM_SETSEL, len, len);
			output = "Process " + std::to_string(id);

			// Если свободного буфера не нашлось, ждем и пробуем снова
			if (!buffer) {
				output += " - Waiting...\r\n";
				output += "-----------------------\r\n";
				SendMessage(hEditOutput, EM_REPLACESEL, 0, (LPARAM)output.c_str());
				std::this_thread::sleep_for(std::chrono::milliseconds(3000));
				continue;
			}

			// Заполнение буфера данными
			auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
			std::string time_str = std::ctime(&now); // Получаем текущее время
			time_str.erase(time_str.length() - 1);   // Убираем символ новой строки

			// Увеличение счетчика для каждой операции
			counter++;

			// Формируем содержимое буфера
			snprintf(buffer->data, BUFFER_SIZE, "Buffer %d - Step %d - Time: %s", buffer->id, 
							counter, time_str.c_str());

			// Вывод данных в элемент управления (окно)
			output += " filled buffer with data:  " + std::string(buffer->data) + "\r\n";
			output += "-----------------------\r\n";
			SendMessage(hEditOutput, EM_REPLACESEL, 0, (LPARAM)output.c_str());

			// Имитация обработки данных
			std::this_thread::sleep_for(std::chrono::milliseconds(3000));

			// Освобождение буфера
			mutex.lock();
			buffer->is_used = false;
			mutex.unlock();
		}
	}

	void StopAllProcesses() {
    	stopFlag = true;  // Устанавливаем флаг остановки
		mutex.lock();
		for (int i = 0; i < BUFFER_COUNT; ++i) {
			SharedBuffer* buffer = shared_memory.getBuffer(i);
			buffer->is_used = false;  // Освобождаем буфер
			memset(buffer->data, 0, BUFFER_SIZE);  // Очищаем данные
		}
		mutex.unlock();

    	// Очищаем поле вывода и выводим сообщение об остановке процессов
    	SetWindowTextA(hEditOutput, "Buffers cleared.\r\nAll processes stopped.\r\n");
	}

};