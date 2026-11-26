#include "fpsdisplay.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

static float fpsTextWidth(const std::string& text, float size) {
    float width = 0.0f;
    for (char c : text) {
        if (c == 'i' || c == 'l' || c == '1' || c == ':' || c == '.' || c == ' ') width += size * 0.3f;
        else if (c == 'm' || c == 'w' || c == 'M' || c == 'W' || c == '/') width += size * 0.8f;
        else width += size * 0.58f;
    }
    return width;
}

FpsDisplayModule::FpsDisplayModule()
    : Module("FPS Display", "Displays the game's measured frame rate and frame time.") {
}

void FpsDisplayModule::onEnable() {
    m_lastFrameAt = {};
    m_windowStart = {};
    m_lastTextUpdate = {};
    m_windowFrames = 0;
    m_currentFps = 0;
    m_frameTimeMs = 0.0f;
    m_displayText = "FPS: --";
}

void FpsDisplayModule::onDisable() {
    m_lastFrameAt = {};
    m_windowStart = {};
    m_lastTextUpdate = {};
    m_windowFrames = 0;
    m_currentFps = 0;
    m_frameTimeMs = 0.0f;
    m_displayText = "FPS: --";
}

void FpsDisplayModule::onFrame() {
    if (!enabled) return;

    const auto now = Clock::now();
    if (m_lastFrameAt.time_since_epoch().count() == 0) {
        m_windowStart = now;
    } else {
        const float deltaMs = std::chrono::duration<float, std::milli>(now - m_lastFrameAt).count();
        if (deltaMs > 0.0f && deltaMs <= 1000.0f) {
            m_frameTimeMs = m_frameTimeMs > 0.0f
                ? (m_frameTimeMs * 0.8f) + (deltaMs * 0.2f)
                : deltaMs;
        } else {
            m_frameTimeMs = 0.0f;
            m_windowStart = now;
            m_windowFrames = 0;
        }
    }
    m_lastFrameAt = now;
    ++m_windowFrames;

    const float elapsedSeconds = std::chrono::duration<float>(now - m_windowStart).count();
    if (elapsedSeconds >= 0.5f) {
        m_currentFps = static_cast<int>(std::lround(static_cast<float>(m_windowFrames) / elapsedSeconds));
        m_windowFrames = 0;
        m_windowStart = now;
    }

    if (m_lastTextUpdate.time_since_epoch().count() == 0 ||
        now - m_lastTextUpdate >= std::chrono::milliseconds(250)) {
        if (m_currentFps > 0) {
            m_displayText = "FPS: " + std::to_string(m_currentFps);
            if (m_showFrameTime && m_frameTimeMs > 0.0f) {
                m_displayText += " | " + std::to_string(static_cast<int>(std::lround(m_frameTimeMs * 10.0f) / 10.0f));
                m_displayText += " ms";
            }
        } else {
            m_displayText = "FPS: --";
        }
        m_lastTextUpdate = now;
    }

    std::vector<PLModMenu_DrawCommand> commands;
    const float boxWidth = fpsTextWidth(m_displayText, m_size) + 12.0f;
    const float boxHeight = m_size + 8.0f;

    if (m_background) {
        PLModMenu_DrawCommand background = {};
        background.type = PL_DRAW_RECT_FILLED;
        background.x = hudPosX;
        background.y = hudPosY;
        background.w = boxWidth;
        background.h = boxHeight;
        const int alpha = static_cast<int>(std::clamp(m_backgroundOpacity, 0.0f, 1.0f) * 255.0f);
        background.color = (alpha << 24);
        commands.push_back(background);
    }

    PLModMenu_DrawCommand text = {};
    text.type = PL_DRAW_TEXT;
    text.x = hudPosX;
    text.y = hudPosY;
    text.w = boxWidth;
    text.h = boxHeight;
    text.color = 0xFFFFFFFF;
    text.size = m_size;
    text.text = m_displayText.c_str();
    commands.push_back(text);

    submitDrawCommands(moduleId, commands);
}

void FpsDisplayModule::loadConfig(const nlohmann::json& j) {
    Module::loadConfig(j);
    if (j.contains("hudPosX") && j["hudPosX"].is_number()) hudPosX = j["hudPosX"].get<float>();
    if (j.contains("hudPosY") && j["hudPosY"].is_number()) hudPosY = j["hudPosY"].get<float>();
    if (j.contains("isHudModule") && j["isHudModule"].is_boolean()) isHudModule = j["isHudModule"].get<bool>();
    if (j.contains("m_size") && j["m_size"].is_number()) m_size = std::clamp(j["m_size"].get<float>(), 8.0f, 96.0f);
    if (j.contains("m_showFrameTime") && j["m_showFrameTime"].is_boolean()) m_showFrameTime = j["m_showFrameTime"].get<bool>();
    if (j.contains("m_background") && j["m_background"].is_boolean()) m_background = j["m_background"].get<bool>();
    if (j.contains("m_backgroundOpacity") && j["m_backgroundOpacity"].is_number()) {
        m_backgroundOpacity = std::clamp(j["m_backgroundOpacity"].get<float>(), 0.0f, 1.0f);
    }
}

void FpsDisplayModule::saveConfig(nlohmann::json& j) {
    Module::saveConfig(j);
    j["hudPosX"] = hudPosX;
    j["hudPosY"] = hudPosY;
    j["isHudModule"] = isHudModule;
    j["m_size"] = m_size;
    j["m_showFrameTime"] = m_showFrameTime;
    j["m_background"] = m_background;
    j["m_backgroundOpacity"] = m_backgroundOpacity;
}
