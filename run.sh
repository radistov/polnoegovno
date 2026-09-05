#!/bin/bash

# Проверка наличия исполняемого файла
if [ ! -f "./build/freelance_parser" ]; then
    echo "Исполняемый файл не найден. Выполните сборку:"
    echo "cd build && cmake .. && make"
    exit 1
fi

# Проверка конфигурации
if [ ! -f "config.json" ]; then
    echo "Файл конфигурации не найден. Создайте config.json"
    exit 1
fi

# Запуск парсера
echo "Запуск Freelance Parser..."
./build/freelance_parser config.json

# Обработка завершения
if [ $? -eq 0 ]; then
    echo "Парсер остановлен корректно"
else
    echo "Парсер завершился с ошибкой"
fi