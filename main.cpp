#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include <csignal>
#include <atomic>
#include "config.h"
#include "db.h"
#include "universal_parser.h"
#include "bot.h"
#include "logger.h"
#include "ai_responder.h"

std::atomic<bool> running(true);

void signalHandler(int signal) {
    Logger::getInstance()->info("Получен сигнал завершения: %d", signal);
    running = false;
}

int main(int argc, char* argv[]) {
    Logger::getInstance()->info("Запуск универсального парсера...");
    
    // Загружаем конфигурацию
    Config config;
    config.loadFromFile("config.json");
    
    // Инициализируем базу данных
    auto db = std::make_shared<Database>(config.db_path);
    db->init();
    
    // Создаем универсальный парсер
    UniversalParser parser(db, config);
    
    // Настраиваем домен для парсинга
    std::string domain = "olx.com"; // Можно передать через аргументы
    if (argc > 1) {
        domain = argv[1];
    }
    
    parser.setDomain(domain);
    
    // Можно указать конкретную категорию
    if (argc > 2) {
        parser.setCategory(argv[2]);
    }
    
    // Создаем бота
    Bot bot(db, config);
    
    // Запускаем бота в отдельном потоке
    std::thread bot_thread([&bot]() {
        bot.start();
    });
    bot_thread.detach();
    
    // Основной цикл
    while (running) {
        try {
            parser.parse();
            
            auto new_orders = parser.getResults();
            
            for (const auto& order : new_orders) {
                Logger::getInstance()->info("Новый заказ: %s (%.2f)", 
                                           order.title.c_str(), order.budget);
                
                // Отправляем уведомление
                bot.sendOrderNotification(order, "");
                
                // Обновляем статус
                db->updateOrderStatus(order.id, "viewed");
            }
            
            // Ждем перед следующим циклом
            for (int i = 0; i < config.parse_interval && running; i++) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            
        } catch (const std::exception& e) {
            Logger::getInstance()->error("Ошибка: %s", e.what());
            std::this_thread::sleep_for(std::chrono::seconds(30));
        }
    }
    
    return 0;
}