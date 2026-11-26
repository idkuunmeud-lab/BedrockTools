#pragma once

#include "../Module.hpp"

#include <array>
#include <string_view>

class PerformanceProfileModule : public Module {
public:
    PerformanceProfileModule();

    void onEnable() override;
    void onDisable() override;

private:
    struct Snapshot {
        std::string_view id;
        bool wasEnabled = false;
        bool found = false;
    };

    std::array<Snapshot, 4> m_snapshot{{
        {"bedrocktools.Motion Blur"},
        {"bedrocktools.Colored Lights"},
        {"bedrocktools.Light Overlay"},
        {"bedrocktools.MapPreview"}
    }};
    bool m_applied = false;
};
