#include <stdio.h>
#include <Mod/CppUserModBase.hpp>

#include "Unreal/AActor.hpp"
#include "Unreal/Hooks.hpp"
#include "Unreal/TArray.hpp"
#include "Unreal/UClass.hpp"

#include "ST_PlayerInfo.hpp"

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
        
        printf("PseudoregaliaMultiplayerMod says hello\n");
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
                // TODO handle connection based on level? if non-gameplay level, close connection if one exists; if
                // gameplay level, establish connection if one doesn't exist
                // Client::OnSceneLoad(actor->GetLevel()->GetName());

                if (!sync_items_hooked)
                {
                    RC::Unreal::UFunction* func = actor->GetFunctionByName(L"SyncInfo");
                    if (!func)
                    {
                        // TODO log error
                        return;
                    }
                    RC::Unreal::UObjectGlobals::RegisterHook(func, sync_info, nop, nullptr);
                    sync_items_hooked = true;
                }
            }
        });
    }

    auto on_update() -> void override
    {
        // TODO poll server and potentially send messages based on current player info
        // Client::Tick();
    }

    static void nop(RC::Unreal::UnrealScriptFunctionCallableContext& context, void* customdata)
    {
    }

    static void sync_info(RC::Unreal::UnrealScriptFunctionCallableContext& context, void* customdata)
    {
        auto& player_info = context.GetParams<FST_PlayerInfo>();
        // TODO use player_info to set info for sending to server
        // Client::SetPlayerInfo(player_info);
        RC::Unreal::TArray<FST_PlayerInfo> ghost_info{};
        // TODO get ghost_info
        // Client::GetGhostInfo(ghost_info);
        context.SetReturnValue(ghost_info);
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
