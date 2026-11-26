#pragma once

#include "../Module.hpp"
#include <atomic>
#include <cstdint>

namespace bedrocktools::sdk { class Player; }

class ProjectileTrajectoryPreviewModule : public Module {
public:
    ProjectileTrajectoryPreviewModule();
    ~ProjectileTrajectoryPreviewModule() override;
    void onInit() override;
    void onEnable() override;
    void onDisable() override;

private:
    void updatePlayer(bedrocktools::sdk::Player* player);
    void render(void* renderer, void* screenContext);
    friend void projectileTrajectoryRenderHook(void*, void*, void*);

    std::atomic<bedrocktools::sdk::Player*> m_player{nullptr};
    void* m_renderTarget = nullptr;
    void* m_tessBegin = nullptr;
    void* m_tessColor = nullptr;
    void* m_tessVertex = nullptr;
    void* m_renderMesh = nullptr;
    void* m_isSolidBlockingBlock = nullptr;
    void* m_hookHandle = nullptr;
    bool m_hookInstalled = false;
    std::uint64_t m_subscription = 0;
};
