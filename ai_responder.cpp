std::string AIIResponder::preparePrompt(const Order& order) {
    std::string prompt = config.response_prompt + "\n\n";
    
    // Добавляем информацию о фрилансере ТОЛЬКО если она есть
    std::string profile_text = config.user_profile.buildProfileText();
    
    if (config.user_profile.isEmpty()) {
        prompt += "ИНФОРМАЦИЯ О ФРИЛАНСЕРЕ:\n";
        prompt += "Данные не указаны. Не выдумывай опыт, навыки или достижения.\n";
        prompt += "Просто напиши заинтересованный отклик без конкретики о себе.\n\n";
    } else {
        prompt += "ИНФОРМАЦИЯ О ФРИЛАНСЕРЕ (используй только эти данные):\n";
        prompt += profile_text + "\n";
        prompt += "ВАЖНО: Не добавляй информацию, которой нет выше. ";
        prompt += "Не выдумывай опыт, портфолио или навыки.\n\n";
    }
    
    // Добавляем информацию о заказе
    prompt += "ИНФОРМАЦИЯ О ЗАКАЗЕ:\n";
    prompt += "Название: " + order.title + "\n";
    if (!order.description.empty()) {
        prompt += "Описание: " + order.description + "\n";
    }
    if (order.budget > 0) {
        prompt += "Бюджет: " + std::to_string(order.budget) + " ₽\n";
    }
    prompt += "Биржа: " + order.exchange + "\n\n";
    
    prompt += "Напиши отклик на этот заказ:";
    
    return prompt;
}