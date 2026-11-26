#include "performanceprofile.hpp"

#include "modules/ModuleRegistry.hpp"

PerformanceProfileModule::PerformanceProfileModule()
    : Module("Low Graphics Profile",
             "Temporarily disables expensive visual effects while enabled. Restores their previous states when disabled.") {
    hideInHudEditor = true;
}

void PerformanceProfileModule::onEnable() {
    auto& registry = ModuleRegistry::get();

    for (auto& entry : m_snapshot) {
        Module* module = registry.find(entry.id);
        entry.found = module != nullptr;
        if (!module) continue;

        entry.wasEnabled = module->masterEnabled;
        if (entry.wasEnabled) module->setMasterEnabled(false);
    }

    m_applied = true;
}

void PerformanceProfileModule::onDisable() {
    if (!m_applied) return;

    auto& registry = ModuleRegistry::get();
    for (auto& entry : m_snapshot) {
        if (!entry.found) continue;
        Module* module = registry.find(entry.id);
        if (!module) continue;

        module->setMasterEnabled(entry.wasEnabled);
    }

    m_applied = false;
}
