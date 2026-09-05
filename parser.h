#pragma once
#include <string>
#include <vector>
#include <memory>
#include <regex>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include "db.h"
#include "config.h"

using json = nlohmann::json;

class Parser {
private:
    Config config;
    std::shared_ptr<Database> db;
    
    // HTTP методы
    std::string httpGet(const std::string& url);
    std::string httpPost(const std::string& url, const std::string& post_data);
    
    // Парсеры бирж
    std::vector<Order> parseKwork();
    std::vector<Order> parseFL();
    std::vector<Order> parseHabr();
    std::vector<Order> parseTelegram();
    
    // Фильтрация
    bool matchesFilters(const Order& order);
    bool matchesKeywords(const Order& order);
    bool matchesBudget(const Order& order);
    
    // Утилиты
    std::string cleanText(const std::string& text);
    std::string extractBudget(const std::string& text);
    std::string takeScreenshot(const std::string& url, const std::string& order_id);
    
    // Парсинг HTML
    std::vector<Order> parseHTML(const std::string& html, const std::string& exchange);
    
public:
    Parser(std::shared_ptr<Database> database, const Config& cfg);
    
    // Основные методы
    void parseAll();
    std::vector<Order> getOrdersForNotification();
    std::vector<Order> getOrdersForAutoApply();
    
    // Дополнительные методы
    void setConfig(const Config& cfg) { config = cfg; }
    Config getConfig() const { return config; }
};