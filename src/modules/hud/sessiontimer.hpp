#pragma once

#include "../Module.hpp"

#include <chrono>
#include <mutex>
#include <string>

class SessionTimerModule : public Module {
public:
    SessionTimerModule();

    void onEnable() override;
    void onDisable() override;
    void onMenuRegistered() override;
    void onFrame() override;
    void loadConfig(const nlohmann::json& json) override;
    void saveConfig(nlohmann::json& json) override;

private:
    using Clock = std::chrono::steady_clock;
    Clock::time_point m_startedAt{};

    mutable std::mutex m_configMutex;
    float hudPosX = 20.0f;
    float hudPosY = 580.0f;
    float m_size = 24.0f;
    bool m_background = true;
    std::string m_backgroundColor = "#000000";
    float m_backgroundOpacity = 0.55f;
    std::string m_textColor = "#FFFFFF";
};
