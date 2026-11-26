#include "projectiletrajectory.hpp"

#include "../ModuleRegistry.hpp"
#include "core/memory/Hooks.hpp"
#include "core/RenderState.hpp"
#include <bedrocktools/events/EventBus.hpp>
#include <bedrocktools/events/LocalPlayerTickEvent.hpp>
#include <bedrocktools/memory/Signatures.hpp>
#include <bedrocktools/sdk/ClientInstance.hpp>
#include <bedrocktools/sdk/Offsets.hpp>
#include <bedrocktools/sdk/world/Actor.hpp>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>

namespace {
using namespace bedrocktools;
using TessBegin = void(*)(void*, void*, int, int, int);
using TessColor = void(*)(void*, float, float, float, float);
using TessVertex = void(*)(void*, float, float, float);
using RenderMesh = void(*)(void*, void*, void*, char*);
static ProjectileTrajectoryPreviewModule* g_trajectory = nullptr;
static void (*g_renderLevelOriginal)(void*, void*, void*) = nullptr;

struct ProjectilePhysics {
    float speed;
    float gravity;
    float drag;
};

bool physicsFor(const std::string& id, ProjectilePhysics& out) {
    if (id.find("ender_pearl") != std::string::npos) { out = {1.5f, 0.03f, 0.99f}; return true; }
    if (id.find("snowball") != std::string::npos) { out = {1.5f, 0.03f, 0.99f}; return true; }
    if (id.find("egg") != std::string::npos) { out = {1.5f, 0.03f, 0.99f}; return true; }
    if (id.find("trident") != std::string::npos) { out = {2.5f, 0.05f, 0.99f}; return true; }
    return false;
}

const std::string* heldItemIdentifier(const bedrocktools::sdk::Player* player) {
    if (!player) return nullptr;
    auto table = *reinterpret_cast<void***>(const_cast<bedrocktools::sdk::Player*>(player));
    if (!table || !table[bedrocktools::sdk::offsets::VTable::PlayerGetCarriedItem]) return nullptr;
    using GetCarriedItem = const void*(*)(const void*);
    const void* stack = reinterpret_cast<GetCarriedItem>(table[bedrocktools::sdk::offsets::VTable::PlayerGetCarriedItem])(player);
    if (!stack || reinterpret_cast<std::uintptr_t>(stack) < 0x10000) return nullptr;
    const auto* counter = *reinterpret_cast<const void* const*>(reinterpret_cast<std::uintptr_t>(stack) + bedrocktools::sdk::offsets::Inventory::ItemStackItemCounter);
    if (!counter) return nullptr;
    const auto* item = *reinterpret_cast<const void* const*>(counter);
    if (!item) return nullptr;
    auto itemTable = *reinterpret_cast<void* const* const*>(item);
    if (!itemTable || !itemTable[bedrocktools::sdk::offsets::Inventory::ItemGetDescriptionIdVtableIndex]) return nullptr;
    using GetDescriptionId = const std::string&(*)(const void*);
    const auto& id = reinterpret_cast<GetDescriptionId>(itemTable[bedrocktools::sdk::offsets::Inventory::ItemGetDescriptionIdVtableIndex])(item);
    return id.size() <= 128 ? &id : nullptr;
}
}

void projectileTrajectoryRenderHook(void* renderer, void* screenContext, void* extra) {
    if (g_renderLevelOriginal) g_renderLevelOriginal(renderer, screenContext, extra);
    if (g_trajectory) g_trajectory->render(renderer, screenContext);
}

ProjectileTrajectoryPreviewModule::ProjectileTrajectoryPreviewModule()
    : Module("Projectile Trajectory Preview",
             "Predict the flight path of held ender pearls, snowballs, eggs and tridents.") {
    hideInHudEditor = true;
    g_trajectory = this;
}

ProjectileTrajectoryPreviewModule::~ProjectileTrajectoryPreviewModule() {
    bedrocktools::events::bus().unsubscribe(m_subscription);
    if (g_trajectory == this) g_trajectory = nullptr;
}

void ProjectileTrajectoryPreviewModule::onInit() {
    m_renderTarget = reinterpret_cast<void*>(bedrocktools::memory::resolve(bedrocktools::memory::SignatureId::RenderLevel));
    m_tessBegin = reinterpret_cast<void*>(bedrocktools::memory::resolve(bedrocktools::memory::SignatureId::TessellatorBegin));
    m_tessColor = reinterpret_cast<void*>(bedrocktools::memory::resolve(bedrocktools::memory::SignatureId::TessellatorColor));
    m_tessVertex = reinterpret_cast<void*>(bedrocktools::memory::resolve(bedrocktools::memory::SignatureId::TessellatorVertex));
    m_renderMesh = reinterpret_cast<void*>(bedrocktools::memory::resolve(bedrocktools::memory::SignatureId::MeshHelpersRenderMeshImmediately2));
    m_subscription = bedrocktools::events::bus().subscribe<bedrocktools::events::LocalPlayerTickEvent>(
        [this](auto& event) { updatePlayer(event.player); });
}

void ProjectileTrajectoryPreviewModule::onEnable() {
    if (m_hookInstalled || !m_renderTarget || !m_tessBegin || !m_tessColor || !m_tessVertex || !m_renderMesh) return;
    m_hookHandle = bedrocktools::hooks::install(m_renderTarget,
        reinterpret_cast<void*>(projectileTrajectoryRenderHook),
        reinterpret_cast<void**>(&g_renderLevelOriginal));
    m_hookInstalled = m_hookHandle != nullptr;
}

void ProjectileTrajectoryPreviewModule::onDisable() {
    m_player = nullptr;
}

void ProjectileTrajectoryPreviewModule::updatePlayer(bedrocktools::sdk::Player* player) {
    m_player = enabled ? player : nullptr;
}

void ProjectileTrajectoryPreviewModule::render(void* renderer, void* screenContext) {
    if (!enabled || !m_player || !renderer || !screenContext ||
        !m_tessBegin || !m_tessColor || !m_tessVertex || !m_renderMesh) return;

    ProjectilePhysics physics{};
    const std::string* itemId = heldItemIdentifier(m_player);
    if (!itemId || !physicsFor(*itemId, physics)) return;

    auto* player = m_player;
    const auto rotation = player->rotation();
    const auto position = player->position();
    const float pitch = rotation.x * 0.01745329251994329577f;
    const float yaw = rotation.y * 0.01745329251994329577f;
    const float cp = std::cos(pitch);
    const float dx = -std::sin(yaw) * cp;
    const float dy = -std::sin(pitch);
    const float dz = std::cos(yaw) * cp;

    bedrocktools::sdk::Vec3 points[81];
    bedrocktools::sdk::Vec3 p{position.x + dx * 0.35f,
                               position.y + 1.50f + dy * 0.35f,
                               position.z + dz * 0.35f};
    bedrocktools::sdk::Vec3 velocity{dx * physics.speed, dy * physics.speed, dz * physics.speed};
    points[0] = p;
    int count = 1;
    for (int tick = 0; tick < 80; ++tick) {
        p.x += velocity.x;
        p.y += velocity.y;
        p.z += velocity.z;
        velocity.x *= physics.drag;
        velocity.y = velocity.y * physics.drag - physics.gravity;
        velocity.z *= physics.drag;
        points[count++] = p;
        if (p.y < -64.0f) break;
    }

    const auto* lrp = *reinterpret_cast<void* const*>(reinterpret_cast<std::uintptr_t>(renderer) +
        bedrocktools::sdk::offsets::LevelRenderer::mLevelRendererPlayer);
    if (!lrp || reinterpret_cast<std::uintptr_t>(lrp) < 0x10000) return;
    const auto camera = *reinterpret_cast<const bedrocktools::sdk::Vec3*>(
        reinterpret_cast<std::uintptr_t>(lrp) + bedrocktools::sdk::offsets::LevelRendererPlayer::mCamPos);
    const auto tess = *reinterpret_cast<void**>(reinterpret_cast<std::uintptr_t>(screenContext) +
        bedrocktools::sdk::offsets::ScreenContext::mTessellator);
    if (!tess || reinterpret_cast<std::uintptr_t>(tess) < 0x10000) return;

    void* material = reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(lrp) +
        bedrocktools::sdk::offsets::LevelRendererPlayer::mSelectionOverlayMaterial);
    bedrocktools::core::ScopedShaderColor shaderColor(screenContext);
    if (!shaderColor) return;

    auto begin = reinterpret_cast<TessBegin>(m_tessBegin);
    auto color = reinterpret_cast<TessColor>(m_tessColor);
    auto vertex = reinterpret_cast<TessVertex>(m_tessVertex);
    auto draw = reinterpret_cast<RenderMesh>(m_renderMesh);
    begin(tess, nullptr, 4, (count - 1) * 2, 0);
    color(tess, 0.12f, 0.92f, 1.0f, 0.95f);
    for (int i = 1; i < count; ++i) {
        const auto& a = points[i - 1];
        const auto& b = points[i];
        vertex(tess, a.x - camera.x, a.y - camera.y, a.z - camera.z);
        vertex(tess, b.x - camera.x, b.y - camera.y, b.z - camera.z);
    }
    char pad[0x58]{};
    draw(screenContext, tess, material, pad);
}
