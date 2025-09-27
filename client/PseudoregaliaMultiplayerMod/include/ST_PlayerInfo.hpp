#ifndef UE4SS_SDK_ST_PlayerInfo_HPP
#define UE4SS_SDK_ST_PlayerInfo_HPP

// id is at the end for alignment reasons; it would take up 8 bytes otherwise
struct FST_PlayerInfo
{
    double location_x;  // 0x0000 (size: 0x8)
    double location_y;  // 0x0008 (size: 0x8)
    double location_z;  // 0x0010 (size: 0x8)
    double rotation_x;  // 0x0018 (size: 0x8)
    double rotation_y;  // 0x0020 (size: 0x8)
    double rotation_z;  // 0x0028 (size: 0x8)
    uint8_t id;         // 0x0030 (size: 0x1)
}; // Size: 0x31

#endif
