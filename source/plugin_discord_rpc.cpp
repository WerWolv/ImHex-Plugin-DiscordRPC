#include <hex/plugin.hpp>

#include <discord.h>

#include <hex/api/content_registry.hpp>
#include <hex/api/event.hpp>
#include <hex/api/localization.hpp>

#include <hex/helpers/logger.hpp>
#include <hex/helpers/utils.hpp>
#include <hex/helpers/fmt.hpp>

#include <hex/providers/provider.hpp>
#include <romfs/romfs.hpp>
#include <nlohmann/json.hpp>

using namespace hex;

constexpr static auto DiscordClientID = 1060827018196955177;
constexpr static auto LargeIconID = "imhex_logo";

static bool g_rpcEnabled = false;
static bool g_showProvider = false;
static bool g_showSelection = false;
static bool g_showTimestamp = false;

static time_t g_startTime = 0;

namespace {

    discord::Core *core = nullptr;

    void updateActivity() {
        if (!g_rpcEnabled) {
            core->ActivityManager().UpdateActivity({}, [](auto) { });
            return;
        }

        static bool updateTimeStamp = true;

        discord::Activity activity = {};
        activity.SetType(discord::ActivityType::Playing);

        if (g_showTimestamp) {
            if (updateTimeStamp) {
                g_startTime = std::time(nullptr);
                updateTimeStamp = false;
            }
            activity.GetTimestamps().SetStart(g_startTime);
        } else {
            activity.GetTimestamps().SetStart(0);
            updateTimeStamp = true;
        }

        if (ImHexApi::Provider::isValid()) {
            std::string state;
            std::string details;

            if (g_showSelection) {
                auto selection = ImHexApi::HexEditor::getSelection();
                if (selection.has_value()) {
                    details += hex::format("[ 0x{0:04x} - 0x{1:04x} ]\n", selection->getStartAddress(), selection->getEndAddress());
                }
            }

            activity.SetDetails(details.c_str());

            if (g_showProvider)
                state = ImHexApi::Provider::get()->getName();

            activity.SetState(state.c_str());
        }

        activity.GetAssets().SetLargeText("v" IMHEX_VERSION);
        activity.GetAssets().SetLargeImage(LargeIconID);

        core->ActivityManager().UpdateActivity(activity, [](discord::Result result) {
            if (result == discord::Result::Ok)
                log::info("Successfully updated activity");
            else
                log::error("Failed to update activity");
        });
    }

    void registerEvents() {
        EventManager::subscribe<EventProviderChanged>([](prv::Provider *, prv::Provider *) {
            updateActivity();
        });

        EventManager::subscribe<EventProviderOpened>([](prv::Provider *) {
            updateActivity();
        });

        EventManager::subscribe<EventRegionSelected>([](Region) {
            updateActivity();
        });

        EventManager::subscribe<EventFrameEnd>([]{
            core->RunCallbacks();
        });

        EventManager::subscribe<EventWindowClosing>([](auto){
            core->ActivityManager().ClearActivity([](discord::Result result) {
                if (result == discord::Result::Ok)
                    log::info("Successfully cleared activity");
                else
                    log::error("Failed to clear activity");
            });
        });

        EventManager::subscribe<EventSettingsChanged>([]{
            if (auto setting = ContentRegistry::Settings::getSetting("hex.discord_rpc.settings", "hex.discord_rpc.settings.enabled"); setting.is_number())
                g_rpcEnabled = setting.get<int>();
            if (auto setting = ContentRegistry::Settings::getSetting("hex.discord_rpc.settings", "hex.discord_rpc.settings.show_provider"); setting.is_number())
                g_showProvider = setting.get<int>();
            if (auto setting = ContentRegistry::Settings::getSetting("hex.discord_rpc.settings", "hex.discord_rpc.settings.show_selection"); setting.is_number())
                g_showSelection = setting.get<int>();
            if (auto setting = ContentRegistry::Settings::getSetting("hex.discord_rpc.settings", "hex.discord_rpc.settings.show_timestamp"); setting.is_number())
                g_showTimestamp = setting.get<int>();

            updateActivity();
        });
    }

    void registerSettings() {
        ContentRegistry::Settings::addCategoryDescription("hex.discord_rpc.settings", "hex.discord_rpc.settings.desc");

        ContentRegistry::Settings::add("hex.discord_rpc.settings", "hex.discord_rpc.settings.enabled", 0, [](const std::string &name, nlohmann::json &setting) {
            bool result = setting.get<int>();
            if (ImGui::Checkbox(LangEntry(name), &result)) {
                setting = int(result);
                return true;
            }

            return false;
        });

        ContentRegistry::Settings::add("hex.discord_rpc.settings", "hex.discord_rpc.settings.show_provider", 0, [](const std::string &name, nlohmann::json &setting) {
            bool result = setting.get<int>();
            ImGui::BeginDisabled(!g_rpcEnabled);
            ON_SCOPE_EXIT { ImGui::EndDisabled(); };
            if (ImGui::Checkbox(LangEntry(name), &result)) {
                setting = int(result);
                return true;
            }

            return false;
        });

        ContentRegistry::Settings::add("hex.discord_rpc.settings", "hex.discord_rpc.settings.show_selection", 0, [](const std::string &name, nlohmann::json &setting) {
            bool result = setting.get<int>();
            ImGui::BeginDisabled(!g_rpcEnabled);
            ON_SCOPE_EXIT { ImGui::EndDisabled(); };
            if (ImGui::Checkbox(LangEntry(name), &result)) {
                setting = int(result);
                return true;
            }

            return false;
        });

        ContentRegistry::Settings::add("hex.discord_rpc.settings", "hex.discord_rpc.settings.show_timestamp", 0, [](const std::string &name, nlohmann::json &setting) {
            bool result = setting.get<int>();
            ImGui::BeginDisabled(!g_rpcEnabled);
            ON_SCOPE_EXIT { ImGui::EndDisabled(); };
            if (ImGui::Checkbox(LangEntry(name), &result)) {
                setting = int(result);
                return true;
            }

            return false;
        });
    }

}

IMHEX_PLUGIN_SETUP("Discord RPC", "WerWolv", "Discord Rich Presence Integration") {
    hex::log::debug("Using romfs: '{}'", romfs::name());
    for (auto &path : romfs::list("lang"))
        hex::ContentRegistry::Language::addLocalization(nlohmann::json::parse(romfs::get(path).string()));

    discord::Core::Create(DiscordClientID, DiscordCreateFlags_Default, &core);

    registerEvents();
    registerSettings();
}


