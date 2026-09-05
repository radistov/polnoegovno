#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <ctime>
#include <iostream>
#include <sstream>

class Logger {
private:
    static Logger* instance;
    static std::mutex mutex;
    std::ofstream log_file;
    bool debug_mode = false;
    
    Logger() {
        log_file.open("parser.log", std::ios::app);
        if (!log_file.is_open()) {
            std::cerr << "Не удалось открыть файл лога!" << std::endl;
        }
    }
    
    std::string getTimestamp() {
        time_t now = time(0);
        struct tm* tstruct = localtime(&now);
        char buf[80];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tstruct);
        return std::string(buf);
    }
    
    void log(const std::string& level, const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex);
        
        std::stringstream ss;
        ss << "[" << getTimestamp() << "] [" << level << "] " << message;
        std::string log_message = ss.str();
        
        if (log_file.is_open()) {
            log_file << log_message << std::endl;
            log_file.flush();
        }
        
        // Выводим в консоль с цветом
        if (level == "ERROR") {
            std::cout << "\033[31m" << log_message << "\033[0m" << std::endl; // Красный
        } else if (level == "WARNING") {
            std::cout << "\033[33m" << log_message << "\033[0m" << std::endl; // Желтый
        } else if (level == "INFO") {
            std::cout << "\033[32m" << log_message << "\033[0m" << std::endl; // Зеленый
        } else if (level == "DEBUG" && debug_mode) {
            std::cout << "\033[36m" << log_message << "\033[0m" << std::endl; // Голубой
        } else {
            std::cout << log_message << std::endl;
        }
    }
    
public:
    static Logger* getInstance() {
        std::lock_guard<std::mutex> lock(mutex);
        if (instance == nullptr) {
            instance = new Logger();
        }
        return instance;
    }
    
    void setDebugMode(bool mode) { debug_mode = mode; }
    
    void info(const std::string& message) {
        log("INFO", message);
    }
    
    void error(const std::string& message) {
        log("ERROR", message);
    }
    
    void warning(const std::string& message) {
        log("WARNING", message);
    }
    
    void debug(const std::string& message) {
        log("DEBUG", message);
    }
    
    // Для логирования с параметрами
    template<typename... Args>
    void info(const std::string& format, Args... args) {
        char buffer[1024];
        snprintf(buffer, sizeof(buffer), format.c_str(), args...);
        info(std::string(buffer));
    }
    
    template<typename... Args>
    void error(const std::string& format, Args... args) {
        char buffer[1024];
        snprintf(buffer, sizeof(buffer), format.c_str(), args...);
        error(std::string(buffer));
    }
};

Logger* Logger::instance = nullptr;
std::mutex Logger::mutex;