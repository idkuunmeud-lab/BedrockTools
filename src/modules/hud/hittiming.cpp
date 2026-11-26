#include "hittiming.hpp"

#include "config/ConfigManager.hpp"
#include <bedrocktools/events/AttackEvent.hpp>
#include <bedrocktools/events/EventBus.hpp>
#include <algorithm>
#include <charconv>
#include <string>
#include <vector>
#include <pl/ModMenu.hpp>

HitTimingModule::HitTimingModule()
    : Module("Hit Timing", "Estimate your next attack window from your own swing timing; server confirmation is not available.") {
}

HitTimingModule::~HitTimingModule() {
    bedrocktools::events::bus().unsubscribe(m_subscription);
}

void HitTimingModule::onInit() {
    m_subscription = bedrocktools::events::bus().subscribe<bedrocktools::events::AttackEvent>(
        [this](auto& event) {
            if (!enabled || !event.target) return;
            const auto now = std::chrono::steady_clock::now();
            std::lock_guard lock(m_mutex);
            if (!m_hasAttack) {
                m_combo = 1;
                m_hasAttack = true;
                m_lastAcceptedAttack = now;
                return;
            }
            const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - m_lastAcceptedAttack).count();
            if (elapsed < 0) {
                m_combo = 1;
                m_lastAcceptedAttack = now;
            } else if (elapsed > m_streakResetMs) {
                m_combo = 1;
                m_lastAcceptedAttack = now;
            } else if (elapsed >= m_cooldownMs) {
                m_combo = std::min(m_combo + 1, 5);
                m_lastAcceptedAttack = now;
            }
        });
}

void HitTimingModule::onEnable() {
    std::lock_guard lock(m_mutex);
    m_hasAttack = false;
    m_combo = 0;
    m_lastAcceptedAttack = {};
}

void HitTimingModule::onDisable() {
    std::lock_guard lock(m_mutex);
    m_hasAttack = false;
    m_combo = 0;
    m_lastAcceptedAttack = {};
    submitDrawCommands(moduleId, {});
}

void HitTimingModule::onFrame() {
    if (!enabled) return;

    int cooldown = 500;
    int combo = 0;
    bool hasAttack = false;
    float progress = 1.0f;
    std::string status = "READY";
    {
        std::lock_guard lock(m_mutex);
        cooldown = m_cooldownMs;
        combo = m_combo;
        hasAttack = m_hasAttack;
        if (hasAttack) {
            const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - m_lastAcceptedAttack).count();
            if (elapsed < 0 || elapsed > m_streakResetMs) {
                status = "READY";
                combo = 0;
                m_combo = 0;
                m_hasAttack = false;
            } else if (elapsed >= cooldown) {
                status = "READY";
                progress = 1.0f;
            } else {
                const auto remaining = cooldown - static_cast<int>(elapsed);
                status = "WAIT " + std::to_string(remaining) + "ms";
                progress = std::clamp(static_cast<float>(elapsed) / static_cast<float>(cooldown), 0.0f, 1.0f);
            }
        }
    }

    constexpr float width = 220.0f;
    constexpr float height = 38.0f;
    constexpr float barHeight = 5.0f;
    std::vector<PLModMenu_DrawCommand> commands;

    PLModMenu_DrawCommand background{};
    background.type = PL_DRAW_RECT_FILLED;
    background.x = hudPosX;
    background.y = hudPosY;
    background.w = width;
    background.h = height;
    background.color = 0xB0000000;
    commands.push_back(background);

    PLModMenu_DrawCommand barBackground{};
    barBackground.type = PL_DRAW_RECT_FILLED;
    barBackground.x = hudPosX + 8.0f;
    barBackground.y = hudPosY + height - barHeight - 6.0f;
    barBackground.w = width - 16.0f;
    barBackground.h = barHeight;
    barBackground.color = 0xFF343434;
    commands.push_back(barBackground);

    PLModMenu_DrawCommand bar{};
    bar.type = PL_DRAW_RECT_FILLED;
    bar.x = hudPosX + 8.0f;
    bar.y = hudPosY + height - barHeight - 6.0f;
    bar.w = (width - 16.0f) * progress;
    bar.h = barHeight;
    bar.color = status == "READY" ? 0xFF49D17D : 0xFFE0A43A;
    commands.push_back(bar);

    const std::string label = m_showStreak && combo > 0
        ? status + "   STREAK " + std::to_string(combo)
        : status;
    PLModMenu_DrawCommand text{};
    text.type = PL_DRAW_TEXT;
    text.x = hudPosX + 8.0f;
    text.y = hudPosY + 4.0f;
    text.w = width - 16.0f;
    text.h = height - barHeight - 8.0f;
    text.size = 22.0f;
    text.color = 0xFFFFFFFF;
    text.text = label.c_str();
    commands.push_back(text);

    submitDrawCommands(moduleId, commands);
}

bool HitTimingModule::onMenuConfigChanged(std::string_view key, std::string_view value) {
    std::lock_guard lock(m_mutex);
    if (key == "cooldownMs") {
        int parsed = m_cooldownMs;
        const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
        if (result.ec != std::errc{} || result.ptr != value.data() + value.size()) return false;
        m_cooldownMs = std::clamp(parsed, 100, 2000);
    } else if (key == "showStreak") {
        m_showStreak = value == "true" || value == "1";
    } else {
        return false;
    }
    bedrocktools::config::ConfigManager::get().save();
    return true;
}

void HitTimingModule::loadConfig(const nlohmann::json& json) {
    Module::loadConfig(json);
    std::lock_guard lock(m_mutex);
    if (json.contains("cooldownMs")) m_cooldownMs = std::clamp(json["cooldownMs"].get<int>(), 100, 2000);
    if (json.contains("showStreak")) m_showStreak = json["showStreak"].get<bool>();
    if (json.contains("hudPosX")) hudPosX = json["hudPosX"].get<float>();
    if (json.contains("hudPosY")) hudPosY = json["hudPosY"].get<float>();
}

void HitTimingModule::saveConfig(nlohmann::json& json) {
    Module::saveConfig(json);
    std::lock_guard lock(m_mutex);
    json["cooldownMs"] = m_cooldownMs;
    json["showStreak"] = m_showStreak;
    json["hudPosX"] = hudPosX;
    json["hudPosY"] = hudPosY;
}

void HitTimingModule::onMenuRegistered() {
    using namespace pl::modmenu;
    ConfigSchemaBuilder schema;
    schema.defaultCategory("timing").category("timing", "Timing", "Local attack timing estimate");
    ConfigNodeV2 cooldown;
    cooldown.id = "cooldownMs";
    cooldown.key = "cooldownMs";
    cooldown.title = "Estimated Cooldown";
    cooldown.description = "Server and weapon rules can differ; this only times your local attack events.";
    cooldown.category = "timing";
    cooldown.type = ConfigControlTypeV2::SliderInt;
    cooldown.minValue = "100";
    cooldown.maxValue = "2000";
    cooldown.step = "25";
    schema.node(std::move(cooldown));

    ConfigNodeV2 streak;
    streak.id = "showStreak";
    streak.key = "showStreak";
    streak.title = "Show Streak";
    streak.category = "timing";
    streak.type = ConfigControlTypeV2::Toggle;
    streak.defaultValue = "true";
    schema.node(std::move(streak));
    setConfigSchemaJson(moduleId, schema.toJson());
}
