#include <Mod/CppUserModBase.hpp>

#include "Unreal/AActor.hpp"
#include "Unreal/FScriptArray.hpp"
#include "Unreal/Hooks.hpp"
#include "Unreal/UClass.hpp"
#include "Unreal/UFunction.hpp"
#include "Unreal/World.hpp"

#include "Client.hpp"
#include "Logger.hpp"
#include "Settings.hpp"

class PseudoregaliaMultiplayerMod : public RC::CppUserModBase
{
public:
    bool sync_items_hooked = false;

    PseudoregaliaMultiplayerMod() : CppUserModBase()
    {
        ModName = STR("PseudoregaliaMultiplayerMod");
        ModVersion = STR("1.1");
        ModDescription = STR("Multiplayer mod for Pseudoregalia");
        ModAuthors = STR("highrow623");
        // Do not change this unless you want to target a UE4SS version
        // other than the one you're currently building with somehow.
        //ModIntendedSDKVersion = STR("2.6");

        Settings::Load();
    }

    ~PseudoregaliaMultiplayerMod() override
    {
    }

    auto on_unreal_init() -> void override
    {
        RC::Unreal::Hook::RegisterBeginPlayPostCallback([&](RC::Unreal::AActor* actor)
        {
            if (actor->GetClassPrivate()->GetName() == L"BP_PM_Manager_C")
            {
                Client::OnSceneLoad(actor->GetWorld()->GetName());

                if (!sync_items_hooked)
                {
                    RC::Unreal::UFunction* func = actor->GetFunctionByName(L"SyncInfo");
                    if (!func)
                    {
                        Log(L"Could not find function \"SyncInfo\" in \"BP_PM_Manager_C\"", LogType::Error);
                        return;
                    }
                    RC::Unreal::UObjectGlobals::RegisterHook(func, sync_info, nop, nullptr);
                    Log(L"Registered hook for \"SyncInfo\" in \"BP_PM_Manager_C\"", LogType::Loud);
                    sync_items_hooked = true;
                }
            }
        });
    }

    auto on_update() -> void override
    {
        Client::Tick();
    }

    static void sync_info(RC::Unreal::UnrealScriptFunctionCallableContext& context, void* customdata)
    {
        const auto& player_info = context.GetParams<FST_PlayerInfo>();
        auto millis = Client::SetPlayerInfo(player_info);

        struct UpdateGhostsParams
        {
            RC::Unreal::FScriptArray ghost_info_raw;
            RC::Unreal::TArray<uint8_t> to_remove;
        };
        auto params = std::make_unique<UpdateGhostsParams>();
        Client::GetGhostInfo(millis, params->ghost_info_raw, params->to_remove);
        if (params->ghost_info_raw.Num() == 0 && params->to_remove.Num() == 0)
        {
            return;
        }

        RC::Unreal::UFunction* update_ghosts = context.Context->GetFunctionByName(L"UpdateGhosts");
        if (!update_ghosts)
        {
            Log(L"Could not find function \"UpdateGhosts\" in \"BP_PM_Manager_C\"", LogType::Error);
            return;
        }
        context.Context->ProcessEvent(update_ghosts, params.get());
    }

    static void nop(RC::Unreal::UnrealScriptFunctionCallableContext& context, void* customdata)
    {
    }
};

#define PSEUDOREGALIA_MULTIPLAYER_MOD_API __declspec(dllexport)
extern "C"
{
    PSEUDOREGALIA_MULTIPLAYER_MOD_API RC::CppUserModBase* start_mod()
    {
        return new PseudoregaliaMultiplayerMod();
    }

    PSEUDOREGALIA_MULTIPLAYER_MOD_API void uninstall_mod(RC::CppUserModBase* mod)
    {
        delete mod;
    }
}
