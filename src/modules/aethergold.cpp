#include "aethergold.hpp"
#include "modules/ModuleRegistry.hpp"
#include "config/ConfigManager.hpp"
#include <pl/ModMenu.hpp>
#include <array>
#include <string_view>
namespace {
struct AetherFeature { std::string_view key,title,description,moduleId,category; };
constexpr std::array<AetherFeature,24> kFeatures{{
{"fps_display","FPS Display","FPS callback-rate estimate and frame-time graph.","bedrocktools.FPS Display","pvp_hud"},
{"ping_counter","Ping Counter","Show the server ping.","bedrocktools.Ping Counter","pvp_hud"},
{"inventory_supply","Inventory Supply","Track pearls, golden apples, obsidian and end crystals.","bedrocktools.Inventory Supply","pvp_hud"},
{"armor_hud","ArmorHUD","Show armor and held-item durability.","bedrocktools.ArmorHUD","pvp_hud"},
{"potion_hud","PotionHUD","Show active effects and timers.","bedrocktools.PotionHUD","pvp_hud"},
{"totem_counter","Totem Counter","Count totems in inventory and offhand.","bedrocktools.Totem Counter","pvp_hud"},
{"keystrokes","Keystrokes","Show key presses and mouse CPS.","bedrocktools.Keystrokes","pvp_hud"},
{"third_person_nametag","Third Person Nametag","Show your own nametag while using third-person view.","bedrocktools.Third Person Nametag","pvp_hud"},
{"arrow_counter","Arrow Counter","Count arrows in inventory and offhand.","bedrocktools.Arrow Counter","pvp_hud"},
{"reach_counter","Reach Counter","Display measured attack reach after a hit.","bedrocktools.Reach Counter","pvp_hud"},
{"combo_display","Combo Display","Track consecutive successful hits.","bedrocktools.Combo Display","pvp_hud"},
{"break_indicator","Break Indicator","Show block-breaking progress.","bedrocktools.Break Indicator","pvp_hud"},
{"tablist","Tablist","Display the loaded player list.","bedrocktools.Tablist","pvp_hud"},
{"compass","Compass","Show facing direction in a compact HUD.","bedrocktools.Compass","utility"},
{"player_coords","Player Coords","Show your XYZ coordinates.","bedrocktools.Player Coords","utility"},
{"speed_display","Speed Display","Show movement speed.","bedrocktools.Speed Display","utility"},
{"session_timer","Session Timer","Show elapsed time since this timer was enabled.","bedrocktools.Session Timer","utility"},
{"world_time","World Time","Display the in-world clock in 12-hour or 24-hour format.","bedrocktools.World Time","utility"},
{"chat_timestamps","Chat Timestamps","Add local timestamps to supported chat and system messages.","bedrocktools.Chat Timestamps","utility"},
{"zoom","Zoom","Smooth camera zoom with configurable FOV.","bedrocktools.Zoom","camera"},
{"freelook","Freelook","Look around without changing your player's facing or movement direction.","bedrocktools.Freelook","camera"},
{"breadcrumbs","Breadcrumbs","Draw a visual trail along your recent movement path.","bedrocktools.Breadcrumbs","visual"},
{"chunk_border","Chunk Border","Draw chunk boundaries for world orientation.","bedrocktools.Chunk Border","visual"},
{"low_graphics","Low Graphics Profile","Temporarily disable selected visual effects to reduce rendering load.","bedrocktools.Low Graphics Profile","performance"}
}};
const AetherFeature* findFeature(std::string_view key) { for (const auto& f:kFeatures) if(f.key==key) return &f; return nullptr; }
}
AetherGoldModule::AetherGoldModule()
: Module("Gold Aether PvP","AetherPvP feature hub. Browse and toggle the included HUD, camera, visual, utility and performance features.") { hideInHudEditor=true; }
void AetherGoldModule::onMenuRegistered() {
using namespace pl::modmenu;
ConfigSchemaBuilder schema;
schema.defaultCategory("pvp_hud")
.category("pvp_hud","PvP HUD","Combat-relevant information overlays")
.category("utility","Utility","Movement, chat and world information")
.category("camera","Camera","Zoom and free-look controls")
.category("visual","Visual","Optional world-space visual aids")
.category("performance","Performance","Graphics and frame pacing options");
auto& registry=ModuleRegistry::get();
for(const auto& f:kFeatures) {
ConfigNodeV2 node; node.id=std::string(f.key); node.key=node.id; node.title=std::string(f.title);
node.description=std::string(f.description); node.category=std::string(f.category);
node.type=ConfigControlTypeV2::Toggle; node.defaultValue="false";
if(Module* module=registry.find(f.moduleId)) { node.currentValue=module->masterEnabled?"true":"false"; node.hasCurrentValue=true; }
else { node.disabled=true; node.description+=" (not available in this build)"; }
schema.node(std::move(node));
}
setConfigSchemaJson(moduleId,schema.toJson());
}
bool AetherGoldModule::onMenuConfigChanged(std::string_view key,std::string_view value) {
const AetherFeature* f=findFeature(key); if(!f) return false;
Module* module=ModuleRegistry::get().find(f->moduleId); if(!module) return true;
module->setMasterEnabled(value=="true"||value=="1"); bedrocktools::config::ConfigManager::get().save(); return true;
}
