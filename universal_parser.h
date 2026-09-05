#pragma once
#include <string>
#include <vector>
#include <memory>
#include <regex>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include "db.h"
#include "config.h"

struct SiteConfig {
    std::string domain;                    // Домен сайта
    std::string category_url;              // URL категории
    std::vector<std::string> selectors;    // CSS селекторы для поиска
    std::string pagination_pattern;        // Паттерн пагинации
    int max_pages = 5;                     // Максимум страниц
    bool use_api = false;                  // Использовать API если есть
    std::string api_endpoint;              // API endpoint
};

class UniversalParser {
private:
    Config config;
    std::shared_ptr<Database> db;
    SiteConfig site_config;
    
    // HTTP методы
    std::string httpGet(const std::string& url);
    std::string httpPost(const std::string& url, const std::string& data);
    
    // Парсинг HTML
    std::vector<Order> parseHTML(const std::string& html);
    std::vector<Order> parseJSON(const std::string& json_str);
    
    // Извлечение данных
    std::string extractTitle(const std::string& html);
    std::string extractDescription(const std::string& html);
    double extractPrice(const std::string& html);
    std::string extractURL(const std::string& html);
    
    // Определение типа сайта
    std::string detectSiteType(const std::string& domain);
    void autoDetectSelectors(const std::string& html);
    
public:
    UniversalParser(std::shared_ptr<Database> database, const Config& cfg);
    
    void setDomain(const std::string& domain);
    void setCategory(const std::string& category);
    void parse();
    std::vector<Order> getResults();
};