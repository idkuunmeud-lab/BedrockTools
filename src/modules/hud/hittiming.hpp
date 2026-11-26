#pragma once

#include "../Module.hpp"
#include <bedrocktools/events/EventBus.hpp>
#include <chrono>
#include <mutex>
#include <string_view>

class HitTimingModule : public Module {
public:
    HitTimingModule();
    ~HitTimingModule() override;
    void onInit() override;
    void onEnable() override;
    void onDisable() override;
    void onFrame() override;
    bool onMenuConfigChanged(std::string_view key, std::string_view value) override;
    void onMenuRegistered() override;
    void loadConfig(const nlohmann::json& json) override;
    void saveConfig(nlohmann::json& json) override;

private:
    std::mutex m_mutex;
    bedrocktools::events::Subscription m_subscription = 0;
    std::chrono::steady_clock::time_point m_lastAcceptedAttack{};
    int m_cooldownMs = 500;
    int m_streakResetMs = 2500;
    int m_combo = 0;
    bool m_hasAttack = false;
    bool m_showStreak = true;
    float hudPosX = 20.0f;
    float hudPosY = 80.0f;
};
