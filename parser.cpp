#include "parser.h"
#include "logger.h"
#include <regex>
#include <cstdlib>
#include <fstream>
#include <algorithm>
#include <cctype>

// Callback для записи ответа от curl
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

Parser::Parser(std::shared_ptr<Database> database, const Config& cfg) 
    : db(database), config(cfg) {
}

std::string Parser::httpGet(const std::string& url) {
    CURL* curl = curl_easy_init();
    std::string response;
    
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            Logger::getInstance()->error("Ошибка HTTP запроса к %s: %s", 
                                        url.c_str(), curl_easy_strerror(res));
        }
        
        curl_easy_cleanup(curl);
    }
    
    return response;
}

std::string Parser::httpPost(const std::string& url, const std::string& post_data) {
    CURL* curl = curl_easy_init();
    std::string response;
    
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            Logger::getInstance()->error("Ошибка POST запроса: %s", curl_easy_strerror(res));
        }
        
        curl_easy_cleanup(curl);
    }
    
    return response;
}

std::string Parser::cleanText(const std::string& text) {
    std::string cleaned = text;
    
    // Убираем HTML теги
    std::regex html_tags("<[^>]*>");
    cleaned = std::regex_replace(cleaned, html_tags, "");
    
    // Убираем лишние пробелы
    std::regex multiple_spaces("\\s+");
    cleaned = std::regex_replace(cleaned, multiple_spaces, " ");
    
    // Убираем пробелы в начале и конце
    cleaned.erase(0, cleaned.find_first_not_of(" \t\n\r"));
    cleaned.erase(cleaned.find_last_not_of(" \t\n\r") + 1);
    
    return cleaned;
}

std::string Parser::extractBudget(const std::string& text) {
    std::regex budget_regex("(\\d[\\d\\s]*)\\s*(?:₽|руб|RUB)", std::regex::icase);
    std::smatch match;
    
    if (std::regex_search(text, match, budget_regex)) {
        std::string budget_str = match[1].str();
        budget_str.erase(std::remove(budget_str.begin(), budget_str.end(), ' '), budget_str.end());
        return budget_str;
    }
    
    return "";
}

bool Parser::matchesBudget(const Order& order) {
    if (order.budget <= 0) {
        return false; // Пропускаем заказы без указанного бюджета
    }
    
    return order.budget >= config.min_budget;
}

bool Parser::matchesKeywords(const Order& order) {
    std::string search_text = order.title + " " + order.description;
    std::transform(search_text.begin(), search_text.end(), search_text.begin(), ::tolower);
    
    for (const auto& keyword : config.keywords) {
        std::string lower_keyword = keyword;
        std::transform(lower_keyword.begin(), lower_keyword.end(), lower_keyword.begin(), ::tolower);
        
        if (search_text.find(lower_keyword) != std::string::npos) {
            return true;
        }
    }
    
    return false;
}

bool Parser::matchesFilters(const Order& order) {
    return matchesBudget(order) && matchesKeywords(order);
}

std::vector<Order> Parser::parseKwork() {
    std::vector<Order> orders;
    Logger::getInstance()->info("Парсинг Kwork...");
    
    // Kwork использует API
    const std::vector<std::string> categories = {
        "razrabotka", "sites", "bots", "design", "texts", 
        "seo", "advertising", "translations", "consultations"
    };
    
    for (const auto& category : categories) {
        std::string url = "https://kwork.ru/projects?category=" + category;
        std::string response = httpGet(url);
        
        // Попытка распарсить JSON (если API)
        try {
            json j = json::parse(response);
            if (j.contains("projects")) {
                for (const auto& project : j["projects"]) {
                    Order order;
                    order.title = project.value("name", "");
                    order.description = project.value("description", "");
                    
                    std::string budget_str = project.value("budget", "0");
                    order.budget = std::stod(budget_str);
                    
                    order.url = "https://kwork.ru" + project.value("url", "");
                    order.exchange = "Kwork";
                    
                    if (matchesFilters(order) && !db->orderExists(order.url)) {
                        orders.push_back(order);
                    }
                }
            }
        } catch (...) {
            // Если не JSON, пробуем парсить HTML
            auto html_orders = parseHTML(response, "Kwork");
            orders.insert(orders.end(), html_orders.begin(), html_orders.end());
        }
        
        // Небольшая задержка между запросами
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    return orders;
}

std::vector<Order> Parser::parseFL() {
    std::vector<Order> orders;
    Logger::getInstance()->info("Парсинг FL.ru...");
    
    const std::vector<std::string> categories = {
        "web-development", "programming", "design", "texts", "marketing"
    };
    
    for (const auto& category : categories) {
        std::string url = "https://www.fl.ru/projects/category/" + category + "/";
        std::string response = httpGet(url);
        
        auto html_orders = parseHTML(response, "FL.ru");
        orders.insert(orders.end(), html_orders.begin(), html_orders.end());
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    return orders;
}

std::vector<Order> Parser::parseHabr() {
    std::vector<Order> orders;
    Logger::getInstance()->info("Парсинг Habr Фриланс...");
    
    std::string url = "https://freelance.habr.com/tasks";
    std::string response = httpGet(url);
    
    auto html_orders = parseHTML(response, "Habr");
    orders.insert(orders.end(), html_orders.begin(), html_orders.end());
    
    return orders;
}

std::vector<Order> Parser::parseTelegram() {
    std::vector<Order> orders;
    Logger::getInstance()->info("Парсинг Telegram-каналов...");
    
    // Здесь должен быть код для парсинга Telegram
    // Требует API credentials от Telegram
    
    return orders;
}

std::vector<Order> Parser::parseHTML(const std::string& html, const std::string& exchange) {
    std::vector<Order> orders;
    
    // Универсальные regex паттерны для поиска заказов
    std::regex project_regex("<article[^>]*>(.*?)</article>|<div[^>]*class=\"[^\"]*project[^\"]*\"[^>]*>(.*?)</div>");
    std::regex title_regex("<a[^>]*class=\"[^\"]*title[^\"]*\"[^>]*>(.*?)</a>|<h2[^>]*>(.*?)</h2>");
    std::regex budget_regex("(\\d[\\d\\s]*)\\s*(?:₽|руб|RUB)", std::regex::icase);
    std::regex url_regex("href=\"([^\"]*)\"");
    
    std::sregex_iterator iter(html.begin(), html.end(), project_regex);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        std::string project_html = iter->str();
        
        Order order;
        std::smatch match;
        
        // Извлекаем заголовок
        if (std::regex_search(project_html, match, title_regex)) {
            order.title = cleanText(match[1].str());
        }
        
        // Извлекаем описание
        std::regex desc_regex("<p[^>]*>(.*?)</p>|<div[^>]*class=\"[^\"]*description[^\"]*\"[^>]*>(.*?)</div>");
        if (std::regex_search(project_html, match, desc_regex)) {
            order.description = cleanText(match[1].str());
        }
        
        // Извлекаем бюджет
        std::string budget_str = extractBudget(project_html);
        if (!budget_str.empty()) {
            order.budget = std::stod(budget_str);
        }
        
        // Извлекаем URL
        if (std::regex_search(project_html, match, url_regex)) {
            order.url = match[1].str();
            if (exchange == "Habr") {
                order.url = "https://freelance.habr.com" + order.url;
            } else if (exchange == "FL.ru") {
                order.url = "https://www.fl.ru" + order.url;
            }
        }
        
        order.exchange = exchange;
        
        if (matchesFilters(order) && !db->orderExists(order.url)) {
            orders.push_back(order);
        }
    }
    
    return orders;
}

std::string Parser::takeScreenshot(const std::string& url, const std::string& order_id) {
    std::string screenshot_path = config.screenshots_dir + "order_" + order_id + ".png";
    
    // Создаем директорию, если её нет
    std::string mkdir_cmd = "mkdir -p " + config.screenshots_dir;
    system(mkdir_cmd.c_str());
    
    // Вызов внешнего скрипта для скриншота
    std::string command = "node " + config.playwright_script + " \"" + url + "\" \"" + screenshot_path + "\"";
    int result = system(command.c_str());
    
    if (result != 0) {
        Logger::getInstance()->error("Не удалось создать скриншот для: %s", url.c_str());
        return "";
    }
    
    return screenshot_path;
}

void Parser::parseAll() {
    std::vector<Order> all_orders;
    
    // Парсим все источники
    auto kwork_orders = parseKwork();
    auto fl_orders = parseFL();
    auto habr_orders = parseHabr();
    auto telegram_orders = parseTelegram();
    
    // Объединяем результаты
    all_orders.insert(all_orders.end(), kwork_orders.begin(), kwork_orders.end());
    all_orders.insert(all_orders.end(), fl_orders.begin(), fl_orders.end());
    all_orders.insert(all_orders.end(), habr_orders.begin(), habr_orders.end());
    all_orders.insert(all_orders.end(), telegram_orders.begin(), telegram_orders.end());
    
    // Сохраняем в базу данных
    int new_count = 0;
    for (auto& order : all_orders) {
        if (db->addOrder(order)) {
            Logger::getInstance()->info("Новый заказ: %s (%.0f ₽)", 
                                       order.title.c_str(), order.budget);
            new_count++;
        }
    }
    
    Logger::getInstance()->info("Парсинг завершен. Найдено новых заказов: %d", new_count);
}

std::vector<Order> Parser::getOrdersForNotification() {
    return db->getNewOrders();
}

std::vector<Order> Parser::getOrdersForAutoApply() {
    return db->getOrdersForAutoApply(config.auto_apply_min_budget);
}