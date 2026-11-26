#include "inventorysupply.hpp"

#include "core/InventoryAccess.hpp"
#include "modules/ModuleRegistry.hpp"
#include <bedrocktools/events/EventBus.hpp>
#include <bedrocktools/sdk/world/Actor.hpp>

#include <algorithm>
#include <charconv>
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

std::uint32_t withOpacity(const std::string& value, float opacity) {
    const auto alpha = static_cast<std::uint32_t>(
        std::clamp(opacity, 0.0f, 1.0f) * 255.0f
    );
    return (alpha << 24U) | parseRgb(value, 0x000000U);
}

}

InventorySupplyModule::InventorySupplyModule()
    : Module("Inventory Supply", "Shows counts for pearls, golden apples, obsidian and end crystals in your inventory.") {
}

InventorySupplyModule::~InventorySupplyModule() {
    bedrocktools::events::bus().unsubscribe(m_subscription);
}

void InventorySupplyModule::onInit() {
    if (m_subscription != 0) return;
    bedrocktools::core::InventoryAccess::get().initialize();
    m_subscription = bedrocktools::events::bus().subscribe<bedrocktools::events::LocalPlayerTickEvent>(
        [this](auto& event) { updateCounts(event.player); }
    );
}

void InventorySupplyModule::updateCounts(bedrocktools::sdk::Player* player) {
    if (!enabled || !player) {
        if (!player) {
            m_pearls.store(0, std::memory_order_release);
            m_goldenApples.store(0, std::memory_order_release);
            m_obsidian.store(0, std::memory_order_release);
            m_crystals.store(0, std::memory_order_release);
        }
        return;
    }

    auto& inventory = bedrocktools::core::InventoryAccess::get();
    m_pearls.store(inventory.countItems(player, "ender_pearl"), std::memory_order_release);
    m_goldenApples.store(inventory.countItems(player, "golden_apple"), std::memory_order_release);
    m_obsidian.store(inventory.countItems(player, "obsidian"), std::memory_order_release);
    m_crystals.store(inventory.countItems(player, "end_crystal"), std::memory_order_release);
}

void InventorySupplyModule::onDisable() {
    m_pearls.store(0, std::memory_order_release);
    m_goldenApples.store(0, std::memory_order_release);
    m_obsidian.store(0, std::memory_order_release);
    m_crystals.store(0, std::memory_order_release);
    submitDrawCommands(moduleId, {});
}

void InventorySupplyModule::onFrame() {
    if (!enabled) return;

    bool showPearls, showGoldenApples, showObsidian, showCrystals, showZero, background;
    float x, y, size, opacity;
    std::string backgroundColor, textColor;
    {
        std::lock_guard lock(m_configMutex);
        showPearls = m_showPearls;
        showGoldenApples = m_showGoldenApples;
        showObsidian = m_showObsidian;
        showCrystals = m_showCrystals;
        showZero = m_showZero;
        background = m_background;
        x = hudPosX;
        y = hudPosY;
        size = std::clamp(m_size, 8.0f, 64.0f);
        opacity = m_backgroundOpacity;
        backgroundColor = m_backgroundColor;
        textColor = m_textColor;
    }

    struct SupplyRow { const char* label; int count; };
    const SupplyRow rows[] = {
        {"Pearls", m_pearls.load(std::memory_order_acquire)},
        {"Golden Apples", m_goldenApples.load(std::memory_order_acquire)},
        {"Obsidian", m_obsidian.load(std::memory_order_acquire)},
        {"End Crystals", m_crystals.load(std::memory_order_acquire)}
    };
    const bool visible[] = {showPearls, showGoldenApples, showObsidian, showCrystals};

    std::vector<std::string> labels;
    labels.reserve(4);
    for (std::size_t i = 0; i < 4; ++i) {
        if (!visible[i] || (!showZero && rows[i].count == 0)) continue;
        labels.emplace_back(std::string(rows[i].label) + ": " + std::to_string(rows[i].count));
    }

    if (labels.empty()) {
        submitDrawCommands(moduleId, {});
        return;
    }

    float width = 0.0f;
    for (const auto& label : labels) width = std::max(width, textWidth(label, size));
    width += 12.0f;
    const float rowHeight = size + 5.0f;
    const float height = rowHeight * static_cast<float>(labels.size()) + 8.0f;

    std::vector<PLModMenu_DrawCommand> commands;
    commands.reserve(labels.size() + (background ? 1 : 0));
    if (background) {
        PLModMenu_DrawCommand box{};
        box.type = PL_DRAW_RECT_FILLED;
        box.x = x;
        box.y = y;
        box.w = width;
        box.h = height;
        box.color = withOpacity(backgroundColor, opacity);
        commands.push_back(box);
    }

    const std::uint32_t foreground = 0xFF000000U | parseRgb(textColor, 0xFFFFFFU);
    for (std::size_t i = 0; i < labels.size(); ++i) {
        PLModMenu_DrawCommand line{};
        line.type = PL_DRAW_TEXT;
        line.x = x + 6.0f;
        line.y = y + 4.0f + static_cast<float>(i) * rowHeight;
        line.w = width - 6.0f;
        line.h = rowHeight;
        line.color = foreground;
        line.size = size;
        line.text = labels[i];
        commands.push_back(line);
    }
    submitDrawCommands(moduleId, commands);
}

void InventorySupplyModule::onMenuRegistered() {
    using namespace pl::modmenu;
    ConfigSchemaBuilder schema;
    schema.defaultCategory("items")
        .category("items", "Items", "Choose which inventory counts to display")
        .category("appearance", "Appearance", "Text and panel style");

    auto node = [](const char* key, const char* title, const char* category, ConfigControlTypeV2 type) {
        ConfigNodeV2 value;
        value.id = key;
        value.key = key;
        value.title = title;
        value.category = category;
        value.type = type;
        return value;
    };

    schema.node(node("m_showPearls", "Ender Pearls", "items", ConfigControlTypeV2::Toggle));
    schema.node(node("m_showGoldenApples", "Golden Apples", "items", ConfigControlTypeV2::Toggle));
    schema.node(node("m_showObsidian", "Obsidian", "items", ConfigControlTypeV2::Toggle));
    schema.node(node("m_showCrystals", "End Crystals", "items", ConfigControlTypeV2::Toggle));
    schema.node(node("m_showZero", "Show Empty Counts", "items", ConfigControlTypeV2::Toggle));
    schema.node(node("keybind", "Keybind", "items", ConfigControlTypeV2::Keybind));

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

void InventorySupplyModule::loadConfig(const nlohmann::json& json) {
    Module::loadConfig(json);
    std::lock_guard lock(m_configMutex);
    if (json.contains("hudPosX") && json["hudPosX"].is_number()) hudPosX = std::clamp(json["hudPosX"].get<float>(), 0.0f, 4000.0f);
    if (json.contains("hudPosY") && json["hudPosY"].is_number()) hudPosY = std::clamp(json["hudPosY"].get<float>(), 0.0f, 4000.0f);
    if (json.contains("m_size") && json["m_size"].is_number()) m_size = std::clamp(json["m_size"].get<float>(), 8.0f, 64.0f);
    if (json.contains("m_showPearls") && json["m_showPearls"].is_boolean()) m_showPearls = json["m_showPearls"].get<bool>();
    if (json.contains("m_showGoldenApples") && json["m_showGoldenApples"].is_boolean()) m_showGoldenApples = json["m_showGoldenApples"].get<bool>();
    if (json.contains("m_showObsidian") && json["m_showObsidian"].is_boolean()) m_showObsidian = json["m_showObsidian"].get<bool>();
    if (json.contains("m_showCrystals") && json["m_showCrystals"].is_boolean()) m_showCrystals = json["m_showCrystals"].get<bool>();
    if (json.contains("m_showZero") && json["m_showZero"].is_boolean()) m_showZero = json["m_showZero"].get<bool>();
    if (json.contains("m_background") && json["m_background"].is_boolean()) m_background = json["m_background"].get<bool>();
    if (json.contains("m_backgroundColor") && json["m_backgroundColor"].is_string()) m_backgroundColor = json["m_backgroundColor"].get<std::string>();
    if (json.contains("m_backgroundOpacity") && json["m_backgroundOpacity"].is_number()) {
        m_backgroundOpacity = std::clamp(json["m_backgroundOpacity"].get<float>(), 0.0f, 1.0f);
    }
    if (json.contains("m_textColor") && json["m_textColor"].is_string()) m_textColor = json["m_textColor"].get<std::string>();
}

void InventorySupplyModule::saveConfig(nlohmann::json& json) {
    Module::saveConfig(json);
    std::lock_guard lock(m_configMutex);
    json["hudPosX"] = hudPosX;
    json["hudPosY"] = hudPosY;
    json["isHudModule"] = true;
    json["m_size"] = m_size;
    json["m_showPearls"] = m_showPearls;
    json["m_showGoldenApples"] = m_showGoldenApples;
    json["m_showObsidian"] = m_showObsidian;
    json["m_showCrystals"] = m_showCrystals;
    json["m_showZero"] = m_showZero;
    json["m_background"] = m_background;
    json["m_backgroundColor"] = m_backgroundColor;
    json["m_backgroundOpacity"] = std::clamp(m_backgroundOpacity, 0.0f, 1.0f);
    json["m_textColor"] = m_textColor;
}
