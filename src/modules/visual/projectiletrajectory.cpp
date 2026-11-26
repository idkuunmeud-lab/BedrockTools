#include "projectiletrajectory.hpp"

#include "core/memory/Hooks.hpp"
#include "core/RenderState.hpp"
#include <bedrocktools/events/EventBus.hpp>
#include <bedrocktools/events/LocalPlayerTickEvent.hpp>
#include <bedrocktools/memory/Signatures.hpp>
#include <bedrocktools/sdk/Offsets.hpp>
#include <bedrocktools/sdk/world/Actor.hpp>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>

namespace {
using namespace bedrocktools;
using TessBegin = void(*)(void*, void*, int, int, int);
using TessColor = void(*)(void*, float, float, float, float);
using TessVertex = void(*)(void*, float, float, float);
using RenderMesh = void(*)(void*, void*, void*, char*);

constexpr float kDegreesToRadians = 0.01745329251994329577f;
constexpr int kMaxFlightTicks = 100;
static ProjectileTrajectoryPreviewModule* g_trajectory = nullptr;
static void (*g_renderLevelOriginal)(void*, void*, void*) = nullptr;

struct ProjectilePhysics {
    float speed;
    float gravity;
    float drag;
};

bool physicsFor(const std::string& id, ProjectilePhysics& out) {
    if (id.find("ender_pearl") != std::string::npos) {
        out = {1.50f, 0.03f, 0.99f};
        return true;
    }
    if (id.find("snowball") != std::string::npos || id.find("egg") != std::string::npos) {
        out = {1.50f, 0.03f, 0.99f};
        return true;
    }
    if (id.find("trident") != std::string::npos) {
        out = {2.50f, 0.05f, 0.99f};
        return true;
    }
    // The SDK currently exposes the held item, but not the active bow-use charge.
    // Use the full-charge launch estimate and make the limitation explicit in the UI.
    if (id.find("bow") != std::string::npos && id.find("crossbow") == std::string::npos) {
        out = {3.00f, 0.05f, 0.99f};
        return true;
    }
    return false;
}

bool heldItemIdentifier(const sdk::Player* player, std::string& result) {
    if (!player) return false;

    auto table = *reinterpret_cast<void***>(const_cast<sdk::Player*>(player));
    if (!table || !table[bedrocktools::sdk::offsets::VTable::PlayerGetCarriedItem]) return false;

    using GetCarriedItem = const void*(*)(const void*);
    const void* stack = reinterpret_cast<GetCarriedItem>(
        table[sdk::offsets::VTable::PlayerGetCarriedItem])(player);
    if (!stack || reinterpret_cast<std::uintptr_t>(stack) < 0x10000) return false;

    const auto* counter = *reinterpret_cast<const void* const*>(
        reinterpret_cast<std::uintptr_t>(stack) + sdk::offsets::Inventory::ItemStackItemCounter);
    if (!counter || reinterpret_cast<std::uintptr_t>(counter) < 0x10000) return false;

    const auto* item = *reinterpret_cast<const void* const*>(counter);
    if (!item || reinterpret_cast<std::uintptr_t>(item) < 0x10000) return false;

    auto itemTable = *reinterpret_cast<void* const* const*>(item);
    if (!itemTable || !itemTable[sdk::offsets::Inventory::ItemGetDescriptionIdVtableIndex]) return false;

    using GetDescriptionId = const std::string&(*)(const void*);
    const auto& id = reinterpret_cast<GetDescriptionId>(
        itemTable[sdk::offsets::Inventory::ItemGetDescriptionIdVtableIndex])(item);
    if (id.empty() || id.size() > 128) return false;

    result = id;
    return true;
}
}

void projectileTrajectoryRenderHook(void* renderer, void* screenContext, void* extra) {
    if (g_renderLevelOriginal) g_renderLevelOriginal(renderer, screenContext, extra);
    if (g_trajectory) g_trajectory->render(renderer, screenContext);
}

ProjectileTrajectoryPreviewModule::ProjectileTrajectoryPreviewModule()
    : Module("Projectile Trajectory Preview",
             "Estimate flight paths for pearls, snowballs, eggs, tridents and fully charged bows.") {
    hideInHudEditor = true;
    g_trajectory = this;
}

ProjectileTrajectoryPreviewModule::~ProjectileTrajectoryPreviewModule() {
    bedrocktools::events::bus().unsubscribe(m_subscription);
    if (m_hookHandle) {
        bedrocktools::hooks::remove(reinterpret_cast<bedrocktools::hooks::Handle>(m_hookHandle));
        m_hookHandle = nullptr;
    }
    m_hookInstalled = false;
    if (g_trajectory == this) g_trajectory = nullptr;
}

void ProjectileTrajectoryPreviewModule::onInit() {
    m_renderTarget = reinterpret_cast<void*>(
        bedrocktools::memory::resolve(bedrocktools::memory::SignatureId::RenderLevel));
    m_tessBegin = reinterpret_cast<void*>(
        bedrocktools::memory::resolve(bedrocktools::memory::SignatureId::TessellatorBegin));
    m_tessColor = reinterpret_cast<void*>(
        bedrocktools::memory::resolve(bedrocktools::memory::SignatureId::TessellatorColor));
    m_tessVertex = reinterpret_cast<void*>(
        bedrocktools::memory::resolve(bedrocktools::memory::SignatureId::TessellatorVertex));
    m_renderMesh = reinterpret_cast<void*>(
        bedrocktools::memory::resolve(bedrocktools::memory::SignatureId::MeshHelpersRenderMeshImmediately2));

    m_subscription = bedrocktools::events::bus().subscribe<bedrocktools::events::LocalPlayerTickEvent>(
        [this](auto& event) { updatePlayer(event.player); });
}

void ProjectileTrajectoryPreviewModule::onEnable() {
    if (m_hookInstalled || !m_renderTarget || !m_tessBegin || !m_tessColor ||
        !m_tessVertex || !m_renderMesh) return;

    m_hookHandle = bedrocktools::hooks::install(
        m_renderTarget,
        reinterpret_cast<void*>(projectileTrajectoryRenderHook),
        reinterpret_cast<void**>(&g_renderLevelOriginal));
    m_hookInstalled = m_hookHandle != nullptr;
}

void ProjectileTrajectoryPreviewModule::onDisable() {
    m_player.store(nullptr, std::memory_order_release);
}

void ProjectileTrajectoryPreviewModule::updatePlayer(bedrocktools::sdk::Player* player) {
    m_player.store(enabled ? player : nullptr, std::memory_order_release);
}

void ProjectileTrajectoryPreviewModule::render(void* renderer, void* screenContext) {
    if (!enabled || !renderer || !screenContext ||
        reinterpret_cast<std::uintptr_t>(renderer) < 0x10000 ||
        reinterpret_cast<std::uintptr_t>(screenContext) < 0x10000 ||
        !m_tessBegin || !m_tessColor || !m_tessVertex || !m_renderMesh) return;

    auto* player = m_player.load(std::memory_order_acquire);
    if (!player) return;

    std::string itemId;
    ProjectilePhysics physics{};
    if (!heldItemIdentifier(player, itemId) || !physicsFor(itemId, physics)) return;

    const auto rotation = player->rotation();
    const auto position = player->position();
    if (!std::isfinite(rotation.x) || !std::isfinite(rotation.y) ||
        !std::isfinite(position.x) || !std::isfinite(position.y) ||
        !std::isfinite(position.z)) return;

    const float pitch = rotation.x * kDegreesToRadians;
    const float yaw = rotation.y * kDegreesToRadians;
    const float cosPitch = std::cos(pitch);
    const float dx = -std::sin(yaw) * cosPitch;
    const float dy = -std::sin(pitch);
    const float dz = std::cos(yaw) * cosPitch;

    std::array<bedrocktools::sdk::Vec3, kMaxFlightTicks + 1> points{};
    bedrocktools::sdk::Vec3 point{
        position.x + dx * 0.25f,
        position.y + 1.52f + dy * 0.25f,
        position.z + dz * 0.25f
    };
    bedrocktools::sdk::Vec3 velocity{dx * physics.speed, dy * physics.speed, dz * physics.speed};
    points[0] = point;
    int count = 1;

    for (int tick = 0; tick < kMaxFlightTicks; ++tick) {
        point.x += velocity.x;
        point.y += velocity.y;
        point.z += velocity.z;

        velocity.x *= physics.drag;
        velocity.y = velocity.y * physics.drag - physics.gravity;
        velocity.z *= physics.drag;

        if (!std::isfinite(point.x) || !std::isfinite(point.y) ||
            !std::isfinite(point.z)) break;

        points[count++] = point;
        if (point.y < -64.0f || point.y > 512.0f) break;
    }
    if (count < 2) return;

    const auto rendererAddress = reinterpret_cast<std::uintptr_t>(renderer);
    const auto* lrp = *reinterpret_cast<void* const*>(
        rendererAddress + sdk::offsets::LevelRenderer::mLevelRendererPlayer);
    if (!lrp || reinterpret_cast<std::uintptr_t>(lrp) < 0x10000) return;

    const auto lrpAddress = reinterpret_cast<std::uintptr_t>(lrp);
    const auto camera = *reinterpret_cast<const bedrocktools::sdk::Vec3*>(
        lrpAddress + sdk::offsets::LevelRendererPlayer::mCamPos);
    if (!std::isfinite(camera.x) || !std::isfinite(camera.y) || !std::isfinite(camera.z)) return;

    const auto contextAddress = reinterpret_cast<std::uintptr_t>(screenContext);
    void* tess = *reinterpret_cast<void**>(
        contextAddress + sdk::offsets::ScreenContext::mTessellator);
    if (!tess || reinterpret_cast<std::uintptr_t>(tess) < 0x10000) return;

    void* material = reinterpret_cast<void*>(
        lrpAddress + sdk::offsets::LevelRendererPlayer::mSelectionOverlayMaterial);
    bedrocktools::core::ScopedShaderColor shaderColor(screenContext);
    if (!shaderColor) return;

    auto begin = reinterpret_cast<TessBegin>(m_tessBegin);
    auto color = reinterpret_cast<TessColor>(m_tessColor);
    auto vertex = reinterpret_cast<TessVertex>(m_tessVertex);
    auto draw = reinterpret_cast<RenderMesh>(m_renderMesh);

    begin(tess, nullptr, 4, (count - 1) * 2, 0);
    color(tess, 0.12f, 0.92f, 1.0f, 0.95f);
    for (int i = 1; i < count; ++i) {
        const auto& a = points[static_cast<std::size_t>(i - 1)];
        const auto& b = points[static_cast<std::size_t>(i)];
        vertex(tess, a.x - camera.x, a.y - camera.y, a.z - camera.z);
        vertex(tess, b.x - camera.x, b.y - camera.y, b.z - camera.z);
    }

    char pad[0x58]{};
    draw(screenContext, tess, material, pad);
}
