#include "projectiletrajectory.hpp"

#include "core/memory/Hooks.hpp"
#include "core/RenderState.hpp"
#include <bedrocktools/events/EventBus.hpp>
#include <bedrocktools/events/LocalPlayerTickEvent.hpp>
#include <bedrocktools/memory/Signatures.hpp>
#include <bedrocktools/sdk/Offsets.hpp>
#include <bedrocktools/sdk/world/Actor.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>

namespace {
using TessBegin = void(*)(void*, void*, int, int, int);
using TessColor = void(*)(void*, float, float, float, float);
using TessVertex = void(*)(void*, float, float, float);
using RenderMesh = void(*)(void*, void*, void*, char*);
using IsSolidBlockingBlock = bool(*)(void*, const bedrocktools::sdk::BlockPos&);

constexpr float kDegreesToRadians = 0.01745329251994329577f;
constexpr int kMaxFlightTicks = 100;
constexpr float kCollisionSampleSpacing = 0.25f;
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
    // The current SDK does not expose active bow-use charge; use a full-charge estimate.
    if (id.find("crossbow") != std::string::npos) {
        out = {3.15f, 0.05f, 0.99f};
        return true;
    }
    if (id.find("bow") != std::string::npos) {
        out = {3.00f, 0.05f, 0.99f};
        return true;
    }
    return false;
}

bool heldItemIdentifier(const bedrocktools::sdk::Player* player, std::string& result) {
    if (!player) return false;

    auto table = *reinterpret_cast<void***>(const_cast<bedrocktools::sdk::Player*>(player));
    if (!table || !table[bedrocktools::sdk::offsets::VTable::PlayerGetCarriedItem]) return false;

    using GetCarriedItem = const void*(*)(const void*);
    const void* stack = reinterpret_cast<GetCarriedItem>(
        table[bedrocktools::sdk::offsets::VTable::PlayerGetCarriedItem])(player);
    if (!stack || reinterpret_cast<std::uintptr_t>(stack) < 0x10000) return false;

    const auto* counter = *reinterpret_cast<const void* const*>(
        reinterpret_cast<std::uintptr_t>(stack) + bedrocktools::sdk::offsets::Inventory::ItemStackItemCounter);
    if (!counter || reinterpret_cast<std::uintptr_t>(counter) < 0x10000) return false;

    const auto* item = *reinterpret_cast<const void* const*>(counter);
    if (!item || reinterpret_cast<std::uintptr_t>(item) < 0x10000) return false;

    auto itemTable = *reinterpret_cast<void* const* const*>(item);
    if (!itemTable || !itemTable[bedrocktools::sdk::offsets::Inventory::ItemGetDescriptionIdVtableIndex]) return false;

    using GetDescriptionId = const std::string&(*)(const void*);
    const auto& id = reinterpret_cast<GetDescriptionId>(
        itemTable[bedrocktools::sdk::offsets::Inventory::ItemGetDescriptionIdVtableIndex])(item);
    if (id.empty() || id.size() > 128) return false;

    result = id;
    return true;
}

bool finite(const bedrocktools::sdk::Vec3& value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
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
    m_isSolidBlockingBlock = reinterpret_cast<void*>(
        bedrocktools::memory::resolve(bedrocktools::memory::SignatureId::BlockSourceIsSolidBlockingBlock));

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
    std::lock_guard lock(m_pathMutex);
    m_pointCount = 0;
    m_hitSolidBlock = false;
}

void ProjectileTrajectoryPreviewModule::updatePlayer(bedrocktools::sdk::Player* player) {
    std::array<bedrocktools::sdk::Vec3, kMaxFlightTicks + 1> nextPoints{};
    int nextCount = 0;
    bool nextHitSolidBlock = false;

    if (!enabled || !player) {
        std::lock_guard lock(m_pathMutex);
        m_points = nextPoints;
        m_pointCount = 0;
        m_hitSolidBlock = false;
        return;
    }

    std::string itemId;
    ProjectilePhysics physics{};
    if (!heldItemIdentifier(player, itemId) || !physicsFor(itemId, physics)) {
        std::lock_guard lock(m_pathMutex);
        m_points = nextPoints;
        m_pointCount = 0;
        m_hitSolidBlock = false;
        return;
    }

    const auto rotation = player->rotation();
    const auto position = player->position();
    if (!std::isfinite(rotation.x) || !std::isfinite(rotation.y) || !finite(position)) {
        std::lock_guard lock(m_pathMutex);
        m_pointCount = 0;
        m_hitSolidBlock = false;
        return;
    }

    const float pitch = rotation.x * kDegreesToRadians;
    const float yaw = rotation.y * kDegreesToRadians;
    const float cosPitch = std::cos(pitch);
    const float dx = -std::sin(yaw) * cosPitch;
    const float dy = -std::sin(pitch);
    const float dz = std::cos(yaw) * cosPitch;

    bedrocktools::sdk::Vec3 point{
        position.x + dx * 0.25f,
        position.y + 1.52f + dy * 0.25f,
        position.z + dz * 0.25f
    };
    bedrocktools::sdk::Vec3 velocity{dx * physics.speed, dy * physics.speed, dz * physics.speed};
    nextPoints[0] = point;
    nextCount = 1;

    void* blockSource = nullptr;
    if (auto* dimension = player->dimension();
        dimension && reinterpret_cast<std::uintptr_t>(dimension) >= 0x10000) {
        blockSource = bedrocktools::sdk::field<void*>(
            dimension, bedrocktools::sdk::offsets::Dimension::mBlockSource);
        if (reinterpret_cast<std::uintptr_t>(blockSource) < 0x10000) blockSource = nullptr;
    }
    auto isSolidBlockingBlock = reinterpret_cast<IsSolidBlockingBlock>(m_isSolidBlockingBlock);

    for (int tick = 0; tick < kMaxFlightTicks; ++tick) {
        const auto previous = point;
        point.x += velocity.x;
        point.y += velocity.y;
        point.z += velocity.z;

        velocity.x *= physics.drag;
        velocity.y = velocity.y * physics.drag - physics.gravity;
        velocity.z *= physics.drag;

        if (!finite(point)) break;

        if (blockSource && isSolidBlockingBlock) {
            const float deltaX = point.x - previous.x;
            const float deltaY = point.y - previous.y;
            const float deltaZ = point.z - previous.z;
            const float distance = std::sqrt(deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ);
            const int steps = std::clamp(
                static_cast<int>(std::ceil(distance / kCollisionSampleSpacing)), 1, 16);
            for (int step = 1; step <= steps; ++step) {
                const float fraction = static_cast<float>(step) / static_cast<float>(steps);
                const bedrocktools::sdk::Vec3 sample{
                    previous.x + deltaX * fraction,
                    previous.y + deltaY * fraction,
                    previous.z + deltaZ * fraction
                };
                const bedrocktools::sdk::BlockPos blockPos{
                    static_cast<int>(std::floor(sample.x)),
                    static_cast<int>(std::floor(sample.y)),
                    static_cast<int>(std::floor(sample.z))
                };
                if (isSolidBlockingBlock(blockSource, blockPos)) {
                    point = sample;
                    nextHitSolidBlock = true;
                    break;
                }
            }
        }

        nextPoints[static_cast<std::size_t>(nextCount++)] = point;
        if (nextHitSolidBlock || point.y < -64.0f || point.y > 512.0f) break;
    }

    std::lock_guard lock(m_pathMutex);
    m_points = nextPoints;
    m_pointCount = nextCount;
    m_hitSolidBlock = nextHitSolidBlock;
}

void ProjectileTrajectoryPreviewModule::render(void* renderer, void* screenContext) {
    if (!enabled || !renderer || !screenContext ||
        reinterpret_cast<std::uintptr_t>(renderer) < 0x10000 ||
        reinterpret_cast<std::uintptr_t>(screenContext) < 0x10000 ||
        !m_tessBegin || !m_tessColor || !m_tessVertex || !m_renderMesh) return;

    std::array<bedrocktools::sdk::Vec3, kMaxFlightTicks + 1> points{};
    int count = 0;
    bool hitSolidBlock = false;
    {
        std::lock_guard lock(m_pathMutex);
        points = m_points;
        count = m_pointCount;
        hitSolidBlock = m_hitSolidBlock;
    }
    if (count < 2) return;

    const auto rendererAddress = reinterpret_cast<std::uintptr_t>(renderer);
    const auto* lrp = *reinterpret_cast<void* const*>(
        rendererAddress + bedrocktools::sdk::offsets::LevelRenderer::mLevelRendererPlayer);
    if (!lrp || reinterpret_cast<std::uintptr_t>(lrp) < 0x10000) return;

    const auto lrpAddress = reinterpret_cast<std::uintptr_t>(lrp);
    const auto camera = *reinterpret_cast<const bedrocktools::sdk::Vec3*>(
        lrpAddress + bedrocktools::sdk::offsets::LevelRendererPlayer::mCamPos);
    if (!finite(camera)) return;

    const auto contextAddress = reinterpret_cast<std::uintptr_t>(screenContext);
    void* tess = *reinterpret_cast<void**>(
        contextAddress + bedrocktools::sdk::offsets::ScreenContext::mTessellator);
    if (!tess || reinterpret_cast<std::uintptr_t>(tess) < 0x10000) return;

    void* material = reinterpret_cast<void*>(
        lrpAddress + bedrocktools::sdk::offsets::LevelRendererPlayer::mSelectionOverlayMaterial);
    bedrocktools::core::ScopedShaderColor shaderColor(screenContext);
    if (!shaderColor) return;

    auto begin = reinterpret_cast<TessBegin>(m_tessBegin);
    auto color = reinterpret_cast<TessColor>(m_tessColor);
    auto vertex = reinterpret_cast<TessVertex>(m_tessVertex);
    auto draw = reinterpret_cast<RenderMesh>(m_renderMesh);

    const int markerVertices = hitSolidBlock ? 6 : 0;
    begin(tess, nullptr, 4, (count - 1) * 2 + markerVertices, 0);
    color(tess, 0.12f, 0.92f, 1.0f, 0.95f);
    for (int i = 1; i < count; ++i) {
        const auto& a = points[static_cast<std::size_t>(i - 1)];
        const auto& b = points[static_cast<std::size_t>(i)];
        vertex(tess, a.x - camera.x, a.y - camera.y, a.z - camera.z);
        vertex(tess, b.x - camera.x, b.y - camera.y, b.z - camera.z);
    }
    if (hitSolidBlock) {
        color(tess, 1.0f, 0.48f, 0.16f, 1.0f);
        const auto& impact = points[static_cast<std::size_t>(count - 1)];
        constexpr float markerSize = 0.16f;
        vertex(tess, impact.x - markerSize - camera.x, impact.y - camera.y, impact.z - camera.z);
        vertex(tess, impact.x + markerSize - camera.x, impact.y - camera.y, impact.z - camera.z);
        vertex(tess, impact.x - camera.x, impact.y - markerSize - camera.y, impact.z - camera.z);
        vertex(tess, impact.x - camera.x, impact.y + markerSize - camera.y, impact.z - camera.z);
        vertex(tess, impact.x - camera.x, impact.y - camera.y, impact.z - markerSize - camera.z);
        vertex(tess, impact.x - camera.x, impact.y - camera.y, impact.z + markerSize - camera.z);
    }

    char pad[0x58]{};
    draw(screenContext, tess, material, pad);
}
