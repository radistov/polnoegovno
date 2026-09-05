#!/bin/bash

echo "=== Установка Freelance Parser with AI ==="

# Обновление системы
echo "Обновление системы..."
sudo apt-get update
sudo apt-get upgrade -y

# Установка системных зависимостей
echo "Установка системных зависимостей..."
sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    libcurl4-openssl-dev \
    libsqlite3-dev \
    libssl-dev \
    libboost-all-dev \
    nlohmann-json3-dev \
    nodejs \
    npm

# Установка TgBot
echo "Установка TgBot..."
if [ ! -d "tgbot-cpp" ]; then
    git clone https://github.com/reo7sp/tgbot-cpp.git
    cd tgbot-cpp
    cmake .
    make -j4
    sudo make install
    cd ..
fi

# Установка Playwright
echo "Установка Playwright..."
npm install playwright
npx playwright install chromium

# Создание директорий
echo "Создание директорий..."
mkdir -p screenshots
mkdir -p build

# Сборка проекта
echo "Сборка проекта..."
cd build
cmake ..
make -j4

echo ""
echo "=== Установка завершена! ==="
echo "1. Отредактируйте config.json"
echo "2. Добавьте токен Telegram бота"
echo "3. Добавьте API ключ DeepSeek"
echo "4. Запустите: ./freelance_parser"