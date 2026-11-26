#pragma once

#include "../Module.hpp"

#include <chrono>
#include <string>

class FpsDisplayModule : public Module {
public:
    FpsDisplayModule();

    void onEnable() override;
    void onDisable() override;
    void onFrame() override;
    void loadConfig(const nlohmann::json& j) override;
    void saveConfig(nlohmann::json& j) override;

private:
    using Clock = std::chrono::steady_clock;

    Clock::time_point m_lastFrameAt{};
    Clock::time_point m_windowStart{};
    Clock::time_point m_lastTextUpdate{};
    unsigned int m_windowFrames = 0;
    int m_currentFps = 0;
    float m_frameTimeMs = 0.0f;
    std::string m_displayText = "FPS: --";

    float hudPosX = 20.0f;
    float hudPosY = 180.0f;
    bool isHudModule = true;
    float m_size = 40.0f;
    bool m_showFrameTime = true;
    bool m_background = false;
    float m_backgroundOpacity = 0.5f;
};
