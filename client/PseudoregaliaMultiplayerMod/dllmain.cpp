#include <Mod/CppUserModBase.hpp>

#include "Unreal/AActor.hpp"
#include "Unreal/Hooks.hpp"
#include "Unreal/UClass.hpp"
#include "Unreal/UFunction.hpp"

#include "Client.hpp"
#include "Logger.hpp"

class PseudoregaliaMultiplayerMod : public RC::CppUserModBase
{
public:
    bool sync_items_hooked = false;

    PseudoregaliaMultiplayerMod() : CppUserModBase()
    {
        ModName = STR("PseudoregaliaMultiplayerMod");
        ModVersion = STR("1.0");
        ModDescription = STR("Multiplayer mod for Pseudoregalia");
        ModAuthors = STR("highrow623");
        // Do not change this unless you want to target a UE4SS version
        // other than the one you're currently building with somehow.
        //ModIntendedSDKVersion = STR("2.6");
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
                Client::OnSceneLoad(actor->GetLevel()->GetName());

                if (!sync_items_hooked)
                {
                    RC::Unreal::UFunction* func = actor->GetFunctionByName(L"SyncInfo");
                    if (!func)
                    {
                        Log(L"Could not find function \"SyncInfo\" in \"BP_PM_Manager_C\"", LogType::Error);
                        return;
                    }
                    RC::Unreal::UObjectGlobals::RegisterHook(func, sync_player_info, sync_ghost_info, nullptr);
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

    static void nop(RC::Unreal::UnrealScriptFunctionCallableContext& context, void* customdata)
    {
    }

    static void sync_player_info(RC::Unreal::UnrealScriptFunctionCallableContext& context, void* customdata)
    {
        auto& player_info = context.GetParams<FST_PlayerInfo>();
        Client::SetPlayerInfo(player_info);
    }

    static void sync_ghost_info(RC::Unreal::UnrealScriptFunctionCallableContext& context, void* customdata)
    {
        RC::Unreal::TArray<FST_PlayerInfo> ghost_info{};
        Client::GetGhostInfo(ghost_info);

        // TODO couldn't get context.SetReturnValue to work after a bit of testing so I've resorted to just calling
        // UpdateGhosts directly from here. I think I'd prefer to use SetReturnValue tho if I can figure that out.
        RC::Unreal::UFunction* update_ghosts = context.Context->GetFunctionByName(L"UpdateGhosts");
        if (!update_ghosts)
        {
            Log(L"Could not find function \"UpdateGhosts\" in \"BP_PM_Manager_C\"", LogType::Error);
            return;
        }
        std::shared_ptr<void> params = std::make_shared<RC::Unreal::TArray<FST_PlayerInfo>>(ghost_info);
        context.Context->ProcessEvent(update_ghosts, params.get());
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
