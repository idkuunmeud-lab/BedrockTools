#pragma once

#include "../Module.hpp"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>

namespace bedrocktools::sdk { class Player; }

class InventorySupplyModule : public Module {
public:
    InventorySupplyModule();
    ~InventorySupplyModule() override;

    void onInit() override;
    void onDisable() override;
    void onMenuRegistered() override;
    void onFrame() override;
    void loadConfig(const nlohmann::json& json) override;
    void saveConfig(nlohmann::json& json) override;

private:
    void updateCounts(bedrocktools::sdk::Player* player);

    std::uint64_t m_subscription = 0;
    std::atomic<int> m_pearls{0};
    std::atomic<int> m_goldenApples{0};
    std::atomic<int> m_obsidian{0};
    std::atomic<int> m_crystals{0};

    mutable std::mutex m_configMutex;
    float hudPosX = 20.0f;
    float hudPosY = 520.0f;
    float m_size = 24.0f;
    bool m_showPearls = true;
    bool m_showGoldenApples = true;
    bool m_showObsidian = true;
    bool m_showCrystals = true;
    bool m_showZero = false;
    bool m_background = true;
    std::string m_backgroundColor = "#000000";
    float m_backgroundOpacity = 0.55f;
    std::string m_textColor = "#FFFFFF";
};
