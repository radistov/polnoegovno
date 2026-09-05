#include "config.h"
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

void Config::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return;
    }
    
    json j;
    try {
        file >> j;
    } catch (...) {
        return;
    }
    
    // Telegram
    if (j.contains("telegram_token")) telegram_token = j["telegram_token"];
    if (j.contains("telegram_chat_id")) telegram_chat_id = j["telegram_chat_id"];
    
    // DeepSeek
    if (j.contains("deepseek_api_key")) deepseek_api_key = j["deepseek_api_key"];
    if (j.contains("deepseek_model")) deepseek_model = j["deepseek_model"];
    if (j.contains("use_ai_autoresponder")) use_ai_autoresponder = j["use_ai_autoresponder"];
    if (j.contains("response_prompt")) response_prompt = j["response_prompt"];
    
    // User Profile - только если указаны
    if (j.contains("user_profile")) {
        auto& profile = j["user_profile"];
        
        if (profile.contains("name") && profile["name"].is_string() && !profile["name"].empty())
            user_profile.name = profile["name"].get<std::string>();
            
        if (profile.contains("skills") && profile["skills"].is_string() && !profile["skills"].empty())
            user_profile.skills = profile["skills"].get<std::string>();
            
        if (profile.contains("experience") && profile["experience"].is_string() && !profile["experience"].empty())
            user_profile.experience = profile["experience"].get<std::string>();
            
        if (profile.contains("portfolio") && profile["portfolio"].is_string() && !profile["portfolio"].empty())
            user_profile.portfolio = profile["portfolio"].get<std::string>();
            
        if (profile.contains("email") && profile["email"].is_string() && !profile["email"].empty())
            user_profile.email = profile["email"].get<std::string>();
            
        if (profile.contains("phone") && profile["phone"].is_string() && !profile["phone"].empty())
            user_profile.phone = profile["phone"].get<std::string>();
            
        if (profile.contains("current_position") && profile["current_position"].is_string() && !profile["current_position"].empty())
            user_profile.current_position = profile["current_position"].get<std::string>();
            
        if (profile.contains("education") && profile["education"].is_string() && !profile["education"].empty())
            user_profile.education = profile["education"].get<std::string>();
    }
    
    // Автоматический режим
    if (j.contains("auto_apply")) auto_apply = j["auto_apply"];
    if (j.contains("auto_apply_min_budget")) auto_apply_min_budget = j["auto_apply_min_budget"];
    if (j.contains("auto_apply_max_per_day")) auto_apply_max_per_day = j["auto_apply_max_per_day"];
    if (j.contains("auto_apply_keywords")) 
        auto_apply_keywords = j["auto_apply_keywords"].get<std::vector<std::string>>();
    
    // Фильтры
    if (j.contains("min_budget")) min_budget = j["min_budget"];
    if (j.contains("keywords")) keywords = j["keywords"].get<std::vector<std::string>>();
    
    // Пути
    if (j.contains("db_path")) db_path = j["db_path"];
    if (j.contains("parse_interval")) parse_interval = j["parse_interval"];
    
    // AI настройки
    if (j.contains("ai_temperature")) ai_temperature = j["ai_temperature"];
    if (j.contains("ai_max_tokens")) ai_max_tokens = j["ai_max_tokens"];
    if (j.contains("ai_tone")) ai_tone = j["ai_tone"];
}

void Config::saveToFile(const std::string& filename) const {
    json j;
    
    // Telegram
    j["telegram_token"] = telegram_token;
    j["telegram_chat_id"] = telegram_chat_id;
    
    // DeepSeek
    j["deepseek_api_key"] = deepseek_api_key;
    j["deepseek_model"] = deepseek_model;
    j["use_ai_autoresponder"] = use_ai_autoresponder;
    j["response_prompt"] = response_prompt;
    
    // User Profile - сохраняем только то, что есть
    json profile = json::object();
    if (user_profile.name) profile["name"] = *user_profile.name;
    if (user_profile.skills) profile["skills"] = *user_profile.skills;
    if (user_profile.experience) profile["experience"] = *user_profile.experience;
    if (user_profile.portfolio) profile["portfolio"] = *user_profile.portfolio;
    if (user_profile.email) profile["email"] = *user_profile.email;
    if (user_profile.phone) profile["phone"] = *user_profile.phone;
    if (user_profile.current_position) profile["current_position"] = *user_profile.current_position;
    if (user_profile.education) profile["education"] = *user_profile.education;
    
    if (!profile.empty()) {
        j["user_profile"] = profile;
    }
    
    // Автоматический режим
    j["auto_apply"] = auto_apply;
    j["auto_apply_min_budget"] = auto_apply_min_budget;
    j["auto_apply_max_per_day"] = auto_apply_max_per_day;
    j["auto_apply_keywords"] = auto_apply_keywords;
    
    // Фильтры
    j["min_budget"] = min_budget;
    j["keywords"] = keywords;
    
    // Пути
    j["db_path"] = db_path;
    j["parse_interval"] = parse_interval;
    
    // AI настройки
    j["ai_temperature"] = ai_temperature;
    j["ai_max_tokens"] = ai_max_tokens;
    j["ai_tone"] = ai_tone;
    
    std::ofstream file(filename);
    file << j.dump(4);
}