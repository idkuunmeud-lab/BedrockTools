#include "sessiontimer.hpp"

#include "modules/ModuleRegistry.hpp"

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace {

float textWidth(const std::string& text, float size) {
    float width = 0.0f;
    for (const char character : text) {
        if (character == 'i' || character == 'l' || character == '1' ||
            character == ':' || character == '.' || character == ' ') {
            width += size * 0.3f;
        } else if (character == 'm' || character == 'w' ||
                   character == 'M' || character == 'W') {
            width += size * 0.8f;
        } else {
            width += size * 0.58f;
        }
    }
    return width;
}

std::uint32_t parseRgb(const std::string& value, std::uint32_t fallback) {
    std::string_view text(value);
    if (!text.empty() && text.front() == '#') text.remove_prefix(1);
    if (text.size() == 8) text.remove_prefix(2);
    if (text.size() != 6) return fallback;
    std::uint32_t result = 0;
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), result, 16);
    if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size()) return fallback;
    return result & 0x00FFFFFFU;
}

std::string formatElapsed(std::chrono::seconds elapsed) {
    auto total = std::max<std::int64_t>(0, elapsed.count());
    const auto days = total / 86400;
    total %= 86400;
    const auto hours = total / 3600;
    total %= 3600;
    const auto minutes = total / 60;
    const auto seconds = total % 60;

    char text[48];
    if (days > 0) {
        std::snprintf(text, sizeof(text), "Session: %lldd %02lld:%02lld:%02lld",
                      static_cast<long long>(days),
                      static_cast<long long>(hours),
                      static_cast<long long>(minutes),
                      static_cast<long long>(seconds));
    } else {
        std::snprintf(text, sizeof(text), "Session: %02lld:%02lld:%02lld",
                      static_cast<long long>(hours),
                      static_cast<long long>(minutes),
                      static_cast<long long>(seconds));
    }
    return text;
}

}

SessionTimerModule::SessionTimerModule()
    : Module("Session Timer", "Displays how long this timer has been enabled in the current Minecraft session.") {
}

void SessionTimerModule::onEnable() {
    m_startedAt = Clock::now();
}

void SessionTimerModule::onDisable() {
    m_startedAt = {};
    submitDrawCommands(moduleId, {});
}

void SessionTimerModule::onFrame() {
    if (!enabled) return;

    float x, y, size, opacity;
    bool background;
    std::string backgroundColor, textColor;
    {
        std::lock_guard lock(m_configMutex);
        x = hudPosX;
        y = hudPosY;
        size = std::clamp(m_size, 8.0f, 64.0f);
        opacity = std::clamp(m_backgroundOpacity, 0.0f, 1.0f);
        background = m_background;
        backgroundColor = m_backgroundColor;
        textColor = m_textColor;
    }

    if (m_startedAt.time_since_epoch().count() == 0) m_startedAt = Clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(Clock::now() - m_startedAt);
    const std::string label = formatElapsed(elapsed);
    const float width = textWidth(label, size) + 16.0f;
    const float height = size + 10.0f;

    std::vector<PLModMenu_DrawCommand> commands;
    commands.reserve(background ? 2 : 1);
    if (background) {
        PLModMenu_DrawCommand box{};
        box.type = PL_DRAW_RECT_FILLED;
        box.x = x;
        box.y = y;
        box.w = width;
        box.h = height;
        box.color = (static_cast<std::uint32_t>(opacity * 255.0f) << 24U) |
                    parseRgb(backgroundColor, 0x000000U);
        commands.push_back(box);
    }

    PLModMenu_DrawCommand text{};
    text.type = PL_DRAW_TEXT;
    text.x = x + 6.0f;
    text.y = y + 4.0f;
    text.w = width - 6.0f;
    text.h = height;
    text.color = 0xFF000000U | parseRgb(textColor, 0xFFFFFFU);
    text.size = size;
    text.text = label;
    commands.push_back(text);

    submitDrawCommands(moduleId, commands);
}

void SessionTimerModule::onMenuRegistered() {
    using namespace pl::modmenu;
    ConfigSchemaBuilder schema;
    schema.defaultCategory("display")
        .category("display", "Display")
        .category("appearance", "Appearance");

    auto node = [](const char* key, const char* title, const char* category, ConfigControlTypeV2 type) {
        ConfigNodeV2 value;
        value.id = key;
        value.key = key;
        value.title = title;
        value.category = category;
        value.type = type;
        return value;
    };

    schema.node(node("keybind", "Keybind", "display", ConfigControlTypeV2::Keybind));
    auto size = node("m_size", "Text Size", "appearance", ConfigControlTypeV2::SliderFloat);
    size.minValue = "8";
    size.maxValue = "64";
    size.step = "1";
    schema.node(std::move(size));

    auto text = node("m_textColor", "Text Color", "appearance", ConfigControlTypeV2::Color);
    text.colorAlpha = false;
    schema.node(std::move(text));

    schema.node(node("m_background", "Background", "appearance", ConfigControlTypeV2::Toggle));
    auto boxColor = node("m_backgroundColor", "Background Color", "appearance", ConfigControlTypeV2::Color);
    boxColor.colorAlpha = false;
    boxColor.visibleWhen = {{"m_background", ConfigConditionOpV2::Truthy, {}}};
    schema.node(std::move(boxColor));

    auto opacity = node("m_backgroundOpacity", "Background Opacity", "appearance", ConfigControlTypeV2::SliderFloat);
    opacity.minValue = "0";
    opacity.maxValue = "1";
    opacity.step = "0.01";
    opacity.visibleWhen = {{"m_background", ConfigConditionOpV2::Truthy, {}}};
    schema.node(std::move(opacity));

    setConfigSchemaJson(moduleId, schema.toJson());
}

void SessionTimerModule::loadConfig(const nlohmann::json& json) {
    Module::loadConfig(json);
    std::lock_guard lock(m_configMutex);
    if (json.contains("hudPosX") && json["hudPosX"].is_number()) hudPosX = std::clamp(json["hudPosX"].get<float>(), 0.0f, 4000.0f);
    if (json.contains("hudPosY") && json["hudPosY"].is_number()) hudPosY = std::clamp(json["hudPosY"].get<float>(), 0.0f, 4000.0f);
    if (json.contains("m_size") && json["m_size"].is_number()) m_size = std::clamp(json["m_size"].get<float>(), 8.0f, 64.0f);
    if (json.contains("m_background") && json["m_background"].is_boolean()) m_background = json["m_background"].get<bool>();
    if (json.contains("m_backgroundColor") && json["m_backgroundColor"].is_string()) m_backgroundColor = json["m_backgroundColor"].get<std::string>();
    if (json.contains("m_backgroundOpacity") && json["m_backgroundOpacity"].is_number()) {
        m_backgroundOpacity = std::clamp(json["m_backgroundOpacity"].get<float>(), 0.0f, 1.0f);
    }
    if (json.contains("m_textColor") && json["m_textColor"].is_string()) m_textColor = json["m_textColor"].get<std::string>();
}

void SessionTimerModule::saveConfig(nlohmann::json& json) {
    Module::saveConfig(json);
    std::lock_guard lock(m_configMutex);
    json["hudPosX"] = hudPosX;
    json["hudPosY"] = hudPosY;
    json["isHudModule"] = true;
    json["m_size"] = m_size;
    json["m_background"] = m_background;
    json["m_backgroundColor"] = m_backgroundColor;
    json["m_backgroundOpacity"] = std::clamp(m_backgroundOpacity, 0.0f, 1.0f);
    json["m_textColor"] = m_textColor;
}
