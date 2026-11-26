#include "aethergold.hpp"

#include "modules/ModuleRegistry.hpp"
#include "config/ConfigManager.hpp"
#include <pl/ModMenu.hpp>
#include <array>
#include <string_view>

namespace {
struct AetherFeature {
    std::string_view key;
    std::string_view title;
    std::string_view description;
    std::string_view moduleId;
    std::string_view category;
};

constexpr std::array<AetherFeature, 11> kFeatures{{
    {"fps_display", "FPS Display", "FPS callback-rate estimate and frame-time graph.", "bedrocktools.FPS Display", "pvp_hud"},
    {"ping_counter", "Ping Counter", "Show the server ping.", "bedrocktools.Ping Counter", "pvp_hud"},
    {"inventory_supply", "Inventory Supply", "Track pearls, golden apples, obsidian and end crystals.", "bedrocktools.Inventory Supply", "pvp_hud"},
    {"armor_hud", "ArmorHUD", "Show armor and held-item durability.", "bedrocktools.ArmorHUD", "pvp_hud"},
    {"potion_hud", "PotionHUD", "Show active effects and timers.", "bedrocktools.PotionHUD", "pvp_hud"},
    {"totem_counter", "Totem Counter", "Count totems in inventory and offhand.", "bedrocktools.Totem Counter", "pvp_hud"},
    {"keystrokes", "Keystrokes", "Show key presses and mouse CPS.", "bedrocktools.Keystrokes", "pvp_hud"},
    {"third_person_nametag", "Third Person Nametag", "Show your own nametag while using third-person view.", "bedrocktools.Third Person Nametag", "pvp_hud"},
    {"player_coords", "Player Coords", "Show your XYZ coordinates.", "bedrocktools.Player Coords", "utility"},
    {"speed_display", "Speed Display", "Show movement speed.", "bedrocktools.Speed Display", "utility"},
    {"session_timer", "Session Timer", "Show elapsed time since this timer was enabled.", "bedrocktools.Session Timer", "utility"},
    {"low_graphics", "Low Graphics Profile", "Temporarily disable selected visual effects to reduce rendering load.", "bedrocktools.Low Graphics Profile", "performance"}
}};

const AetherFeature* findFeature(std::string_view key) {
    for (const auto& feature : kFeatures) {
        if (feature.key == key) return &feature;
    }
    return nullptr;
}
}

AetherGoldModule::AetherGoldModule()
    : Module("Gold Aether PvP", "AetherPvP feature hub. Open this entry to browse and toggle the included HUD, utility and performance features.") {
    hideInHudEditor = true;
}

void AetherGoldModule::onMenuRegistered() {
    using namespace pl::modmenu;
    ConfigSchemaBuilder schema;
    schema.defaultCategory("pvp_hud")
        .category("pvp_hud", "PvP HUD", "Combat-relevant information overlays")
        .category("utility", "Utility", "Movement and session information")
        .category("performance", "Performance", "Graphics and frame pacing options");

    auto& registry = ModuleRegistry::get();
    for (const auto& feature : kFeatures) {
        ConfigNodeV2 node;
        node.id = std::string(feature.key);
        node.key = node.id;
        node.title = std::string(feature.title);
        node.description = std::string(feature.description);
        node.category = std::string(feature.category);
        node.type = ConfigControlTypeV2::Toggle;
        node.defaultValue = "false";
        if (Module* module = registry.find(feature.moduleId)) {
            node.currentValue = module->masterEnabled ? "true" : "false";
            node.hasCurrentValue = true;
        } else {
            node.disabled = true;
            node.description += " (not available in this build)";
        }
        schema.node(std::move(node));
    }

    setConfigSchemaJson(moduleId, schema.toJson());
}

bool AetherGoldModule::onMenuConfigChanged(std::string_view key, std::string_view value) {
    const AetherFeature* feature = findFeature(key);
    if (!feature) return false;

    Module* module = ModuleRegistry::get().find(feature->moduleId);
    if (!module) return true;

    const bool enabled = value == "true" || value == "1";
    module->setMasterEnabled(enabled);
    bedrocktools::config::ConfigManager::get().save();
    return true;
}
