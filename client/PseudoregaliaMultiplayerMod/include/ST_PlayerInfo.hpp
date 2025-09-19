#ifndef UE4SS_SDK_ST_PlayerInfo_HPP
#define UE4SS_SDK_ST_PlayerInfo_HPP

#include "Unreal/FString.hpp"

struct FST_PlayerInfo
{
    RC::Unreal::FString id;   // 0x0000 (size: 0x10)
    RC::Unreal::FString zone; // 0x0010 (size: 0x10)
    int64_t location_x;       // 0x0020 (size: 0x8)
    int64_t location_y;       // 0x0028 (size: 0x8)
    int64_t location_z;       // 0x0030 (size: 0x8)
    int64_t rotation_x;       // 0x0038 (size: 0x8)
    int64_t rotation_y;       // 0x0040 (size: 0x8)
    int64_t rotation_z;       // 0x0048 (size: 0x8)
    int64_t scale_x;          // 0x0050 (size: 0x8)
    int64_t scale_y;          // 0x0058 (size: 0x8)
    int64_t scale_z;          // 0x0060 (size: 0x8)
}; // Size: 0x68

#endif
