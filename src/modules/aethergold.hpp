#pragma once

#include "Module.hpp"

class AetherGoldModule : public Module {
public:
    AetherGoldModule();

    void onMenuRegistered() override;
    bool onMenuConfigChanged(std::string_view key, std::string_view value) override;
};
