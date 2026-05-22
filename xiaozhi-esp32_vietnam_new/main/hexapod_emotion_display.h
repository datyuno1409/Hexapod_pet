#ifndef HEXAPOD_EMOTION_DISPLAY_H
#define HEXAPOD_EMOTION_DISPLAY_H

#include <string>

/**
 * @brief Hexapod Emotion Display
 *
 * Displays emotional expressions on the OLED display.
 * Supports: happy, sad, curious, excited, neutral, confused, angry
 */
class HexapodEmotionDisplay {
public:
    static HexapodEmotionDisplay& GetInstance();

    /**
     * @brief Show emotion on display
     * @param emotion Emotion name (happy, sad, curious, excited, neutral, etc.)
     * @param text Optional text to display below emotion
     */
    void ShowEmotion(const std::string& emotion, const std::string& text = "");

    /**
     * @brief Show text message
     * @param text Message to display
     */
    void ShowText(const std::string& text);

    /**
     * @brief Clear display
     */
    void Clear();

    /**
     * @brief Animate emotion
     * @param emotion Emotion name
     * @param duration_ms Animation duration in milliseconds
     */
    void AnimateEmotion(const std::string& emotion, uint32_t duration_ms = 1000);

private:
    HexapodEmotionDisplay();
    ~HexapodEmotionDisplay() = default;

    HexapodEmotionDisplay(const HexapodEmotionDisplay&) = delete;
    HexapodEmotionDisplay& operator=(const HexapodEmotionDisplay&) = delete;

    std::string current_emotion_;

    /**
     * @brief Draw eyes pattern based on emotion
     * @param emotion Emotion name
     */
    void DrawEyes(const std::string& emotion);

    /**
     * @brief Draw mouth pattern based on emotion
     * @param emotion Emotion name
     */
    void DrawMouth(const std::string& emotion);
};

#endif  // HEXAPOD_EMOTION_DISPLAY_H
