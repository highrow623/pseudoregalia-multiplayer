#ifndef UE4SS_SDK_ST_PlayerInfo_HPP
#define UE4SS_SDK_ST_PlayerInfo_HPP

struct FST_PlayerInfo
{
    int64_t id;         // 0x0000 (size: 0x8)
    double location_x;  // 0x0008 (size: 0x8)
    double location_y;  // 0x0010 (size: 0x8)
    double location_z;  // 0x0018 (size: 0x8)
    double rotation_x;  // 0x0020 (size: 0x8)
    double rotation_y;  // 0x0028 (size: 0x8)
    double rotation_z;  // 0x0030 (size: 0x8)
    double scale_x;     // 0x0038 (size: 0x8)
    double scale_y;     // 0x0040 (size: 0x8)
    double scale_z;     // 0x0048 (size: 0x8)
}; // Size: 0x50

#endif
