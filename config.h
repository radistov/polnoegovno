#pragma once
#include <string>
#include <vector>
#include <map>
#include <optional>

struct UserProfile {
    // Основная информация (опционально)
    std::optional<std::string> name;           // Имя (если указано)
    std::optional<std::string> skills;          // Навыки (если указаны)
    std::optional<std::string> experience;      // Опыт (если указан)
    std::optional<std::string> portfolio;       // Портфолио (если есть)
    std::optional<std::string> email;           // Email для связи
    std::optional<std::string> phone;           // Телефон
    
    // Дополнительно
    std::optional<std::string> current_position; // Текущая должность
    std::optional<std::string> education;        // Образование
    
    bool isEmpty() const {
        return !name && !skills && !experience && !portfolio && 
               !email && !phone && !current_position && !education;
    }
    
    std::string buildProfileText() const {
        std::string result;
        
        if (name) {
            result += "Имя: " + *name + "\n";
        }
        if (current_position) {
            result += "Должность: " + *current_position + "\n";
        }
        if (skills) {
            result += "Навыки: " + *skills + "\n";
        }
        if (experience) {
            result += "Опыт: " + *experience + "\n";
        }
        if (education) {
            result += "Образование: " + *education + "\n";
        }
        if (portfolio) {
            result += "Портфолио: " + *portfolio + "\n";
        }
        if (email) {
            result += "Email: " + *email + "\n";
        }
        if (phone) {
            result += "Телефон: " + *phone + "\n";
        }
        
        return result.empty() ? "Информация о фрилансере не указана" : result;
    }
};

struct Config {
    // Telegram настройки
    std::string telegram_token = "YOUR_BOT_TOKEN";
    long long telegram_chat_id = 123456789;
    
    // DeepSeek API настройки
    std::string deepseek_api_key = "YOUR_DEEPSEEK_API_KEY";
    std::string deepseek_model = "deepseek-chat";
    bool use_ai_autoresponder = true;
    
    // Промпт для генерации откликов
    std::string response_prompt = 
        "Ты - фрилансер, который пишет отклики на заказы. "
        "Напиши заинтересованный отклик на заказ. "
        "Используй ТОЛЬКО информацию, которая предоставлена о фрилансере. "
        "НЕ ВЫДУМЫВАЙ опыт, навыки, портфолио или достижения. "
        "Если информации о фрилансере нет - просто напиши о заинтересованности в проекте "
        "и готовности обсудить детали. "
        "Отклик должен быть кратким (2-3 предложения) и деловым.";
    
    // Профиль пользователя (может быть пустым)
    UserProfile user_profile;
    
    // Автоматический режим
    bool auto_apply = false;
    double auto_apply_min_budget = 10000.0;
    int auto_apply_max_per_day = 10;
    std::vector<std::string> auto_apply_keywords = {"сайт", "бот"};
    
    // Фильтры
    double min_budget = 5000.0;
    std::vector<std::string> keywords = {
        "сайт", "бот", "разработка", "лендинг", "парсер", "дизайн"
    };
    
    // Пути
    std::string db_path = "orders.db";
    std::string screenshots_dir = "./screenshots/";
    std::string playwright_script = "./screenshot.js";
    
    // Интервалы
    int parse_interval = 300;
    
    // AI настройки
    float ai_temperature = 0.7;
    int ai_max_tokens = 500;
    std::string ai_tone = "professional";
    
    void loadFromFile(const std::string& filename);
    void saveToFile(const std::string& filename) const;
};