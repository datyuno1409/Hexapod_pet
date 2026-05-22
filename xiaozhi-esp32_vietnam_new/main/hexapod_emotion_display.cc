#include "hexapod_emotion_display.h"
#include <esp_log.h>

#define TAG "EmotionDisplay"

HexapodEmotionDisplay& HexapodEmotionDisplay::GetInstance() {
    static HexapodEmotionDisplay instance;
    return instance;
}

HexapodEmotionDisplay::HexapodEmotionDisplay() : current_emotion_("neutral") {
}

void HexapodEmotionDisplay::ShowEmotion(const std::string& emotion, const std::string& text) {
    ESP_LOGI(TAG, "Showing emotion: %s, text: %s", emotion.c_str(), text.c_str());

    current_emotion_ = emotion;

    // Draw emotion on OLED
    DrawEyes(emotion);
    DrawMouth(emotion);

    if (!text.empty()) {
        ShowText(text);
    }
}

void HexapodEmotionDisplay::ShowText(const std::string& text) {
    ESP_LOGI(TAG, "Showing text: %s", text.c_str());
    // TODO: Render text on display
}

void HexapodEmotionDisplay::Clear() {
    ESP_LOGI(TAG, "Clearing display");
    // TODO: Clear OLED display
}

void HexapodEmotionDisplay::AnimateEmotion(const std::string& emotion, uint32_t duration_ms) {
    ESP_LOGI(TAG, "Animating emotion: %s for %d ms", emotion.c_str(), duration_ms);
    // TODO: Implement emotion animation
}

void HexapodEmotionDisplay::DrawEyes(const std::string& emotion) {
    ESP_LOGV(TAG, "Drawing eyes for emotion: %s", emotion.c_str());

    // Different eye patterns for different emotions
    if (emotion == "happy") {
        // Happy: happy eyes (>_<)
        ESP_LOGI(TAG, "Drawing happy eyes");
    } else if (emotion == "sad") {
        // Sad: sad eyes (;_;)
        ESP_LOGI(TAG, "Drawing sad eyes");
    } else if (emotion == "curious") {
        // Curious: raised eyebrows (?_?)
        ESP_LOGI(TAG, "Drawing curious eyes");
    } else if (emotion == "excited") {
        // Excited: wide open eyes (o_o)
        ESP_LOGI(TAG, "Drawing excited eyes");
    } else if (emotion == "confused") {
        // Confused: twisted eyes
        ESP_LOGI(TAG, "Drawing confused eyes");
    } else if (emotion == "angry") {
        // Angry: angry eyes >:(
        ESP_LOGI(TAG, "Drawing angry eyes");
    } else {
        // Neutral: neutral eyes
        ESP_LOGI(TAG, "Drawing neutral eyes");
    }
}

void HexapodEmotionDisplay::DrawMouth(const std::string& emotion) {
    ESP_LOGV(TAG, "Drawing mouth for emotion: %s", emotion.c_str());

    // Different mouth patterns for different emotions
    if (emotion == "happy") {
        // Happy: smiling mouth
        ESP_LOGI(TAG, "Drawing smiling mouth");
    } else if (emotion == "sad") {
        // Sad: frowning mouth
        ESP_LOGI(TAG, "Drawing sad mouth");
    } else if (emotion == "curious") {
        // Curious: neutral mouth
        ESP_LOGI(TAG, "Drawing curious mouth");
    } else if (emotion == "excited") {
        // Excited: open mouth
        ESP_LOGI(TAG, "Drawing excited mouth");
    } else if (emotion == "confused") {
        // Confused: confused mouth
        ESP_LOGI(TAG, "Drawing confused mouth");
    } else if (emotion == "angry") {
        // Angry: angry mouth
        ESP_LOGI(TAG, "Drawing angry mouth");
    } else {
        // Neutral: neutral mouth
        ESP_LOGI(TAG, "Drawing neutral mouth");
    }
}
