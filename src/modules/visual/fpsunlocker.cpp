#include "fpsunlocker.hpp"
#include <EGL/egl.h>
#include <time.h>
#include <errno.h>
#include <algorithm>
#include <utility>
#include <pl/ModMenu.hpp>
#include <pl/ModMenuConfig.hpp>

FPSUnlockerModule::FPSUnlockerModule()
    : Module("FPS Unlocker", "Temporarily disabled: the previous EGL swap-interval override and callback sleep caused unstable frame pacing on Android.") {}

void FPSUnlockerModule::onInit() {}

void FPSUnlockerModule::onEnable() {
    // Fail closed: do not disable VSync or sleep inside the frame callback.
    m_appliedDisplay = EGL_NO_DISPLAY;
    m_appliedSurface = EGL_NO_SURFACE;
    m_hasLastFrame = false;
}

void FPSUnlockerModule::onDisable() {
    EGLDisplay display = eglGetCurrentDisplay();
    if (display != EGL_NO_DISPLAY) eglSwapInterval(display, 1);
    m_appliedDisplay = EGL_NO_DISPLAY;
    m_appliedSurface = EGL_NO_SURFACE;
    m_hasLastFrame = false;
}

void FPSUnlockerModule::onFrame() {
    // Deliberately no-op until a display-synchronized frame pacing path is available.
}


void FPSUnlockerModule::onMenuRegistered() {
    using namespace pl::modmenu;
    ConfigSchemaBuilder schema;
    schema.defaultCategory("pacing")
        .category("pacing", "Frame Pacing", "Limit frame output to reduce heat and power use");

    ConfigNodeV2 target;
    target.id = "targetFPS";
    target.key = "targetFPS";
    target.title = "FPS Limit";
    target.description = "0 disables the limiter. A lower cap can reduce heat and power use; it does not increase the device's maximum FPS.";
    target.category = "pacing";
    target.type = ConfigControlTypeV2::SliderInt;
    target.minValue = "0";
    target.maxValue = "240";
    target.step = "15";
    schema.node(std::move(target));

    setConfigSchemaJson(moduleId, schema.toJson());
}

void FPSUnlockerModule::loadConfig(const nlohmann::json& j) {
    Module::loadConfig(j);
    if (j.contains("targetFPS")) targetFPS = std::clamp(j["targetFPS"].get<int>(), 0, 240);
}

void FPSUnlockerModule::saveConfig(nlohmann::json& j) {
    Module::saveConfig(j);
    j["targetFPS"] = targetFPS;
}
