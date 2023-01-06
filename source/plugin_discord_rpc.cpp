#include <hex/plugin.hpp>

#include <discord.h>

#include <hex/api/content_registry.hpp>
#include <hex/api/event.hpp>
#include <hex/api/localization.hpp>

#include <hex/helpers/logger.hpp>
#include <hex/helpers/fmt.hpp>

#include <hex/providers/provider.hpp>
#include <romfs/romfs.hpp>

using namespace hex;

constexpr static auto DiscordClientID = 1060827018196955177;
constexpr static auto LargeIconID = "imhex_logo";

namespace {

    discord::Core *core = nullptr;

    void updateActivity(const std::string &state = "") {
        discord::Activity activity = {};
        activity.SetType(discord::ActivityType::Watching);
        if (!state.empty())
            activity.SetState(state.c_str());
        activity.GetAssets().SetLargeText(IMHEX_VERSION);
        activity.GetAssets().SetLargeImage(LargeIconID);

        core->ActivityManager().UpdateActivity(activity, [](discord::Result result) {
            if (result == discord::Result::Ok)
                log::info("Successfully updated activity");
            else
                log::error("Failed to update activity");
        });
    }

    void updateDetails(prv::Provider *provider) {
        if (provider == nullptr)
            updateActivity();
        else
            updateActivity(hex::format("hex.discord_rpc.analyzing"_lang, provider->getName()));
    }

    void registerEvents() {
        EventManager::subscribe<EventProviderChanged>([](prv::Provider *, prv::Provider *newProvider) {
            updateDetails(newProvider);
        });

        EventManager::subscribe<EventProviderOpened>(updateDetails);

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
    }

}

IMHEX_PLUGIN_SETUP("Discord RPC", "WerWolv", "Discord Rich Presence Integration") {
    hex::log::debug("Using romfs: '{}'", romfs::name());
    for (auto &path : romfs::list("lang"))
        hex::ContentRegistry::Language::addLocalization(nlohmann::json::parse(romfs::get(path).string()));

    discord::Core::Create(DiscordClientID, DiscordCreateFlags_Default, &core);
    updateActivity();

    registerEvents();
}


