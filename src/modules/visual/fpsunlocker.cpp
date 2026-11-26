#include "fpsunlocker.hpp"
#include <EGL/egl.h>
#include <time.h>
#include <errno.h>
#include <algorithm>
#include <utility>
#include <pl/ModMenu.hpp>
#include <pl/ModMenuConfig.hpp>

FPSUnlockerModule::FPSUnlockerModule()
    : Module("FPS Unlocker", "Unlocks frame rate beyond VSync. Set target FPS or 0 for unlimited.") {}

void FPSUnlockerModule::onInit() {}

void FPSUnlockerModule::onEnable() {
    m_appliedDisplay = EGL_NO_DISPLAY;
    m_appliedSurface = EGL_NO_SURFACE;
    m_hasLastFrame = false;
}

void FPSUnlockerModule::onDisable() {
    EGLDisplay display = eglGetCurrentDisplay();
    if (display != EGL_NO_DISPLAY) {
        eglSwapInterval(display, 1);
    }
    m_appliedDisplay = EGL_NO_DISPLAY;
    m_appliedSurface = EGL_NO_SURFACE;
    m_hasLastFrame = false;
}

void FPSUnlockerModule::onFrame() {
    EGLDisplay display = eglGetCurrentDisplay();
    EGLSurface surface = eglGetCurrentSurface(EGL_DRAW);
    if (display != EGL_NO_DISPLAY && surface != EGL_NO_SURFACE &&
        (display != m_appliedDisplay || surface != m_appliedSurface)) {
        if (eglSwapInterval(display, 0) == EGL_TRUE) {
            m_appliedDisplay = display;
            m_appliedSurface = surface;
        }
    }

    if (targetFPS <= 0) {
        m_hasLastFrame = false;
        return;
    }

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    if (m_hasLastFrame) {
        long long frameDurationNs = 1000000000LL / targetFPS;

        struct timespec deadline;
        deadline.tv_sec = m_lastFrameTime.tv_sec;
        deadline.tv_nsec = m_lastFrameTime.tv_nsec + frameDurationNs;

        while (deadline.tv_nsec >= 1000000000L) {
            deadline.tv_sec++;
            deadline.tv_nsec -= 1000000000L;
        }

        while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &deadline, nullptr) == EINTR) {
        }

        m_lastFrameTime = deadline;
    } else {
        m_lastFrameTime = now;
        m_hasLastFrame = true;
    }
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
