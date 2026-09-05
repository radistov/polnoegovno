#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include <csignal>
#include <atomic>
#include <fstream>
#include "config.h"
#include "db.h"
#include "parser.h"
#include "bot.h"
#include "logger.h"
#include "ai_responder.h"

std::atomic<bool> running(true);

void signalHandler(int signal) {
    Logger::getInstance()->info("Получен сигнал завершения: %d", signal);
    running = false;
}

void printBanner() {
    std::cout << "\033[36m"; // Голубой цвет
    std::cout << "╔════════════════════════════════════════╗" << std::endl;
    std::cout << "║     Freelance Parser with AI v1.0      ║" << std::endl;
    std::cout << "║  Мониторинг заказов и автопилот       ║" << std::endl;
    std::cout << "╚════════════════════════════════════════╝" << std::endl;
    std::cout << "\033[0m" << std::endl;
}

void checkConfig(const Config& config) {
    Logger::getInstance()->info("Проверка конфигурации...");
    
    if (config.telegram_token == "YOUR_BOT_TOKEN") {
        Logger::getInstance()->warning("Токен Telegram не настроен!");
    }
    
    if (config.deepseek_api_key == "YOUR_DEEPSEEK_API_KEY") {
        Logger::getInstance()->warning("API ключ DeepSeek не настроен!");
    }
    
    Logger::getInstance()->info("Минимальный бюджет: %.0f ₽", config.min_budget);
    Logger::getInstance()->info("Ключевые слова: %zu", config.keywords.size());
    
    if (config.user_profile.isEmpty()) {
        Logger::getInstance()->info("Профиль пользователя не заполнен");
        Logger::getInstance()->info("AI будет генерировать отклики без личной информации");
    } else {
        Logger::getInstance()->info("Профиль пользователя заполнен");
    }
    
    if (config.auto_apply) {
        Logger::getInstance()->info("Авто-отклики ВКЛЮЧЕНЫ");
        Logger::getInstance()->info("Максимум откликов в день: %d", config.auto_apply_max_per_day);
    } else {
        Logger::getInstance()->info("Авто-отклики выключены");
    }
}

int main(int argc, char* argv[]) {
    printBanner();
    
    // Устанавливаем обработчики сигналов
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    Logger::getInstance()->info("Запуск парсера фриланс-заказов...");
    
    // Загружаем конфигурацию
    Config config;
    std::string config_file = "config.json";
    
    // Проверяем аргументы командной строки
    if (argc > 1) {
        config_file = argv[1];
    }
    
    if (std::ifstream(config_file).good()) {
        config.loadFromFile(config_file);
        Logger::getInstance()->info("Конфигурация загружена из %s", config_file.c_str());
    } else {
        Logger::getInstance()->warning("Файл конфигурации %s не найден. Используем значения по умолчанию.", 
                                      config_file.c_str());
        config.saveToFile(config_file);
        Logger::getInstance()->info("Создан шаблон конфигурации: %s", config_file.c_str());
    }
    
    checkConfig(config);
    
    // Инициализируем базу данных
    auto db = std::make_shared<Database>(config.db_path);
    if (!db->init()) {
        Logger::getInstance()->error("Не удалось инициализировать базу данных");
        return 1;
    }
    
    // Создаем парсер
    Parser parser(db, config);
    
    // Создаем AI responder
    AIIResponder ai_responder(db, config);
    
    // Создаем бота
    Bot bot(db, config);
    
    // Запускаем бота в отдельном потоке
    std::thread bot_thread([&bot]() {
        try {
            bot.start();
        } catch (const std::exception& e) {
            Logger::getInstance()->error("Ошибка в потоке бота: %s", e.what());
        }
    });
    bot_thread.detach();
    Logger::getInstance()->info("Telegram-бот запущен в фоновом режиме");
    
    // Счетчики для авто-откликов
    int auto_applied_today = db->getTodayApplicationsCount();
    Logger::getInstance()->info("Сегодня уже отправлено откликов: %d", auto_applied_today);
    
    // Основной цикл парсинга
    while (running) {
        try {
            Logger::getInstance()->info("--- Начало цикла парсинга ---");
            
            // Парсим заказы
            parser.parseAll();
            
            // Получаем новые заказы для уведомления
            auto new_orders = parser.getOrdersForNotification();
            
            // Отправляем уведомления
            for (const auto& order : new_orders) {
                // Создаем скриншот (опционально)
                std::string screenshot = "";
                // screenshot = parser.takeScreenshot(order.url, std::to_string(order.id));
                
                // Отправляем уведомление
                bot.sendOrderNotification(order, screenshot);
                
                // Обновляем статус
                db->updateOrderStatus(order.id, "viewed");
                
                // Небольшая задержка между уведомлениями
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
            
            // Проверяем авто-отклики
            if (config.auto_apply) {
                auto auto_orders = parser.getOrdersForAutoApply();
                
                for (const auto& order : auto_orders) {
                    // Проверяем лимит
                    if (auto_applied_today >= config.auto_apply_max_per_day) {
                        Logger::getInstance()->warning("Достигнут лимит авто-откликов на сегодня: %d", 
                                                      config.auto_apply_max_per_day);
                        break;
                    }
                    
                    // Проверяем ключевые слова для авто-отклика
                    bool keyword_match = false;
                    std::string search_text = order.title + " " + order.description;
                    std::transform(search_text.begin(), search_text.end(), 
                                 search_text.begin(), ::tolower);
                    
                    for (const auto& keyword : config.auto_apply_keywords) {
                        std::string lower_keyword = keyword;
                        std::transform(lower_keyword.begin(), lower_keyword.end(), 
                                     lower_keyword.begin(), ::tolower);
                        
                        if (search_text.find(lower_keyword) != std::string::npos) {
                            keyword_match = true;
                            break;
                        }
                    }
                    
                    if (!keyword_match) {
                        continue;
                    }
                    
                    // Генерируем и отправляем отклик
                    Logger::getInstance()->info("Авто-отклик на заказ: %s", order.title.c_str());
                    
                    std::string response = ai_responder.generateOrderResponse(order);
                    if (!response.empty()) {
                        if (ai_responder.sendResponseToOrder(order, response)) {
                            auto_applied_today++;
                            Logger::getInstance()->info("Авто-отклик отправлен (%d/%d)", 
                                                      auto_applied_today, 
                                                      config.auto_apply_max_per_day);
                            
                            // Отправляем уведомление в Telegram
                            std::string notify_msg = "🤖 Автоматический отклик отправлен:\n\n";
                            notify_msg += "Заказ: " + order.title + "\n";
                            notify_msg += "Бюджет: " + std::to_string(order.budget) + " ₽\n";
                            notify_msg += "Отклик: " + response.substr(0, 200) + "...";
                            
                            // Здесь нужно отправить через bot
                        }
                    }
                    
                    // Задержка между авто-откликами
                    std::this_thread::sleep_for(std::chrono::seconds(5));
                }
            }
            
            // Ждем перед следующим циклом
            Logger::getInstance()->info("Ожидание %d секунд до следующего парсинга...", 
                                       config.parse_interval);
            
            for (int i = 0; i < config.parse_interval && running; i++) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            
        } catch (const std::exception& e) {
            Logger::getInstance()->error("Ошибка в основном цикле: %s", e.what());
            std::this_thread::sleep_for(std::chrono::seconds(30));
        }
    }
    
    Logger::getInstance()->info("Парсер остановлен");
    return 0;
}