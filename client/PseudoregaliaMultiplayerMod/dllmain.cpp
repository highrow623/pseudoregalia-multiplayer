#include <stdio.h>
#include <Mod/CppUserModBase.hpp>

class PseudoregaliaMultiplayerMod : public RC::CppUserModBase
{
public:
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

    auto on_update() -> void override
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
