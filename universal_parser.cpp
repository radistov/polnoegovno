#include "universal_parser.h"
#include "logger.h"
#include <algorithm>
#include <cctype>

UniversalParser::UniversalParser(std::shared_ptr<Database> database, const Config& cfg)
    : db(database), config(cfg) {
}

void UniversalParser::setDomain(const std::string& domain) {
    site_config.domain = domain;
    
    // Нормализация домена
    if (site_config.domain.find("http://") != 0 && 
        site_config.domain.find("https://") != 0) {
        site_config.domain = "https://" + site_config.domain;
    }
    
    // Определяем тип сайта
    std::string site_type = detectSiteType(domain);
    Logger::getInstance()->info("Определен тип сайта: %s", site_type.c_str());
    
    // Автоматическая настройка для известных сайтов
    if (domain.find("olx") != std::string::npos) {
        site_config.category_url = "/uslugi/";
        site_config.selectors = {
            "div[data-cy='l-card']",           // Карточка объявления
            "div.css-1sw7q4x",                  // Альтернативный селектор
            "div.offer-wrapper"                 // Запасной вариант
        };
        site_config.pagination_pattern = "?page={page}";
        site_config.max_pages = 10;
    }
    else if (domain.find("avito") != std::string::npos) {
        site_config.category_url = "/uslugi";
        site_config.selectors = {
            "div[data-marker='item']",
            "div.iva-item-root",
            "div.item_table"
        };
        site_config.pagination_pattern = "?p={page}";
        site_config.max_pages = 10;
    }
    else if (domain.find("youla") != std::string::npos) {
        site_config.category_url = "/uslugi";
        site_config.selectors = {
            "div[data-test-component='ProductOrAdCard']",
            "div.product-list__item"
        };
        site_config.pagination_pattern = "/p{page}";
        site_config.max_pages = 5;
    }
    else {
        // Универсальные селекторы для неизвестных сайтов
        site_config.category_url = "/";
        site_config.selectors = {
            "article", "div.card", "div.product", 
            "div.item", "div.listing-item", "div[class*='card']",
            "div[class*='item']", "div[class*='product']",
            "div[class*='listing']", "div[class*='offer']"
        };
        site_config.pagination_pattern = "?page={page}";
        site_config.max_pages = 3;
    }
}

void UniversalParser::setCategory(const std::string& category) {
    if (site_config.domain.find("olx") != std::string::npos) {
        site_config.category_url = "/uslugi/" + category + "/";
    }
    else if (site_config.domain.find("avito") != std::string::npos) {
        site_config.category_url = "/" + category;
    }
    else {
        site_config.category_url = "/" + category + "/";
    }
}

std::string UniversalParser::detectSiteType(const std::string& domain) {
    if (domain.find("olx") != std::string::npos) return "classified";
    if (domain.find("avito") != std::string::npos) return "classified";
    if (domain.find("youla") != std::string::npos) return "classified";
    if (domain.find("kwork") != std::string::npos) return "freelance";
    if (domain.find("fl.ru") != std::string::npos) return "freelance";
    if (domain.find("habr") != std::string::npos) return "freelance";
    if (domain.find("freelance") != std::string::npos) return "freelance";
    return "unknown";
}

void UniversalParser::autoDetectSelectors(const std::string& html) {
    // Автоматическое определение селекторов на основе HTML
    Logger::getInstance()->info("Автоматическое определение селекторов...");
    
    // Ищем повторяющиеся блоки
    std::vector<std::string> potential_selectors;
    
    // Проверяем common-паттерны
    std::regex patterns[] = {
        std::regex("<article[^>]*>"),
        std::regex("<div[^>]*class=\"[^\"]*card[^\"]*\"[^>]*>"),
        std::regex("<div[^>]*class=\"[^\"]*item[^\"]*\"[^>]*>"),
        std::regex("<div[^>]*class=\"[^\"]*product[^\"]*\"[^>]*>"),
        std::regex("<div[^>]*class=\"[^\"]*listing[^\"]*\"[^>]*>"),
        std::regex("<div[^>]*class=\"[^\"]*offer[^\"]*\"[^>]*>"),
        std::regex("<li[^>]*class=\"[^\"]*item[^\"]*\"[^>]*>"),
        std::regex("<div[^>]*data-testid=\"[^\"]*card[^\"]*\"[^>]*>")
    };
    
    for (const auto& pattern : patterns) {
        std::sregex_iterator iter(html.begin(), html.end(), pattern);
        std::sregex_iterator end;
        
        int count = std::distance(iter, end);
        if (count >= 3) { // Минимум 3 повторения
            std::smatch match;
            std::regex_search(html, match, pattern);
            potential_selectors.push_back(match.str());
        }
    }
    
    if (!potential_selectors.empty()) {
        site_config.selectors = potential_selectors;
        Logger::getInstance()->info("Найдено %zu селекторов", potential_selectors.size());
    }
}

std::vector<Order> UniversalParser::parseHTML(const std::string& html) {
    std::vector<Order> orders;
    
    // Если селекторы не настроены, пробуем автоопределение
    if (site_config.selectors.empty()) {
        autoDetectSelectors(html);
    }
    
    // Пробуем разные селекторы
    for (const auto& selector : site_config.selectors) {
        // Преобразуем CSS селектор в regex (упрощенно)
        std::string regex_pattern;
        
        if (selector.find("article") != std::string::npos) {
            regex_pattern = "<article[^>]*>(.*?)</article>";
        }
        else if (selector.find("data-cy") != std::string::npos) {
            std::string attr = selector.substr(selector.find("data-cy=") + 9);
            attr = attr.substr(0, attr.find("]"));
            regex_pattern = "<div[^>]*data-cy=\"" + attr + "\"[^>]*>(.*?)</div>";
        }
        else if (selector.find("class") != std::string::npos) {
            std::string class_name = selector.substr(selector.find("class=") + 7);
            class_name = class_name.substr(0, class_name.find("\""));
            regex_pattern = "<div[^>]*class=\"[^\"]*" + class_name + "[^\"]*\"[^>]*>(.*?)</div>";
        }
        else {
            continue;
        }
        
        std::regex block_regex(regex_pattern, std::regex::icase);
        std::sregex_iterator iter(html.begin(), html.end(), block_regex);
        std::sregex_iterator end;
        
        for (; iter != end; ++iter) {
            std::string block_html = iter->str();
            
            Order order;
            
            // Извлекаем заголовок
            order.title = extractTitle(block_html);
            
            // Извлекаем описание
            order.description = extractDescription(block_html);
            
            // Извлекаем цену
            order.budget = extractPrice(block_html);
            
            // Извлекаем URL
            order.url = extractURL(block_html);
            
            // Нормализуем URL
            if (!order.url.empty() && order.url[0] == '/') {
                order.url = site_config.domain + order.url;
            }
            
            order.exchange = site_config.domain;
            
            // Проверяем фильтры
            if (!order.title.empty() && !db->orderExists(order.url)) {
                // Проверяем ключевые слова
                std::string search_text = order.title + " " + order.description;
                std::transform(search_text.begin(), search_text.end(), 
                             search_text.begin(), ::tolower);
                
                bool keyword_match = false;
                for (const auto& keyword : config.keywords) {
                    std::string lower_keyword = keyword;
                    std::transform(lower_keyword.begin(), lower_keyword.end(), 
                                 lower_keyword.begin(), ::tolower);
                    
                    if (search_text.find(lower_keyword) != std::string::npos) {
                        keyword_match = true;
                        break;
                    }
                }
                
                // Проверяем бюджет
                bool budget_match = true;
                if (order.budget > 0 && order.budget < config.min_budget) {
                    budget_match = false;
                }
                
                if (keyword_match && budget_match) {
                    orders.push_back(order);
                }
            }
        }
        
        // Если нашли заказы, прекращаем поиск
        if (!orders.empty()) {
            break;
        }
    }
    
    return orders;
}

std::string UniversalParser::extractTitle(const std::string& html) {
    std::vector<std::regex> patterns = {
        std::regex("<h[1-6][^>]*>(.*?)</h[1-6]>", std::regex::icase),
        std::regex("<a[^>]*class=\"[^\"]*title[^\"]*\"[^>]*>(.*?)</a>", std::regex::icase),
        std::regex("<a[^>]*class=\"[^\"]*name[^\"]*\"[^>]*>(.*?)</a>", std::regex::icase),
        std::regex("<span[^>]*class=\"[^\"]*title[^\"]*\"[^>]*>(.*?)</span>", std::regex::icase),
        std::regex("<div[^>]*class=\"[^\"]*title[^\"]*\"[^>]*>(.*?)</div>", std::regex::icase)
    };
    
    for (const auto& pattern : patterns) {
        std::smatch match;
        if (std::regex_search(html, match, pattern)) {
            std::string title = match[1].str();
            // Очищаем от HTML тегов
            std::regex tags("<[^>]*>");
            title = std::regex_replace(title, tags, "");
            // Очищаем от лишних пробелов
            std::regex spaces("\\s+");
            title = std::regex_replace(title, spaces, " ");
            return title;
        }
    }
    
    return "";
}

std::string UniversalParser::extractDescription(const std::string& html) {
    std::vector<std::regex> patterns = {
        std::regex("<p[^>]*>(.*?)</p>", std::regex::icase),
        std::regex("<div[^>]*class=\"[^\"]*description[^\"]*\"[^>]*>(.*?)</div>", std::regex::icase),
        std::regex("<div[^>]*class=\"[^\"]*desc[^\"]*\"[^>]*>(.*?)</div>", std::regex::icase),
        std::regex("<span[^>]*class=\"[^\"]*description[^\"]*\"[^>]*>(.*?)</span>", std::regex::icase)
    };
    
    for (const auto& pattern : patterns) {
        std::smatch match;
        if (std::regex_search(html, match, pattern)) {
            std::string desc = match[1].str();
            std::regex tags("<[^>]*>");
            desc = std::regex_replace(desc, tags, "");
            std::regex spaces("\\s+");
            desc = std::regex_replace(desc, spaces, " ");
            return desc;
        }
    }
    
    return "";
}

double UniversalParser::extractPrice(const std::string& html) {
    std::vector<std::regex> patterns = {
        std::regex("(\\d[\\d\\s]*)\\s*(?:₽|руб|RUB|USD|EUR)", std::regex::icase),
        std::regex("(\\d[\\d\\s]*)\\s*(?:грн|тенге|бел)", std::regex::icase),
        std::regex("цена[^\\d]*(\\d[\\d\\s]*)", std::regex::icase),
        std::regex("price[^\\d]*(\\d[\\d\\s]*)", std::regex::icase)
    };
    
    for (const auto& pattern : patterns) {
        std::smatch match;
        if (std::regex_search(html, match, pattern)) {
            std::string price_str = match[1].str();
            price_str.erase(std::remove(price_str.begin(), price_str.end(), ' '), price_str.end());
            try {
                return std::stod(price_str);
            } catch (...) {
                return 0;
            }
        }
    }
    
    return 0;
}

std::string UniversalParser::extractURL(const std::string& html) {
    std::regex url_pattern("href=\"([^\"]*)\"", std::regex::icase);
    std::smatch match;
    
    if (std::regex_search(html, match, url_pattern)) {
        return match[1].str();
    }
    
    return "";
}

void UniversalParser::parse() {
    Logger::getInstance()->info("Парсинг сайта: %s", site_config.domain.c_str());
    
    for (int page = 1; page <= site_config.max_pages; page++) {
        std::string url = site_config.domain + site_config.category_url;
        
        // Добавляем пагинацию
        if (page > 1) {
            std::string pagination = site_config.pagination_pattern;
            size_t pos = pagination.find("{page}");
            if (pos != std::string::npos) {
                pagination.replace(pos, 6, std::to_string(page));
                url += pagination;
            }
        }
        
        Logger::getInstance()->debug("Загрузка страницы: %s", url.c_str());
        std::string html = httpGet(url);
        
        if (html.empty()) {
            Logger::getInstance()->warning("Пустой ответ от %s", url.c_str());
            break;
        }
        
        auto orders = parseHTML(html);
        
        if (orders.empty() && page == 1) {
            // Пробуем автоопределение селекторов
            autoDetectSelectors(html);
            orders = parseHTML(html);
        }
        
        int new_orders = 0;
        for (const auto& order : orders) {
            if (db->addOrder(order)) {
                new_orders++;
            }
        }
        
        Logger::getInstance()->info("Страница %d: найдено %zu заказов, новых: %d", 
                                   page, orders.size(), new_orders);
        
        if (orders.empty()) {
            break; // Нет больше заказов
        }
        
        // Задержка между страницами
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

std::vector<Order> UniversalParser::getResults() {
    return db->getNewOrders();
}