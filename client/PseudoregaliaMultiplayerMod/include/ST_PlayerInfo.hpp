#ifndef UE4SS_SDK_ST_PlayerInfo_HPP
#define UE4SS_SDK_ST_PlayerInfo_HPP

// the uint8_t fields are at the end for alignment reasons
struct FST_PlayerInfo
{
    double location_x;  // 0x0000 (size: 0x8)
    double location_y;  // 0x0008 (size: 0x8)
    double location_z;  // 0x0010 (size: 0x8)
    double rotation_x;  // 0x0018 (size: 0x8)
    double rotation_y;  // 0x0020 (size: 0x8)
    double rotation_z;  // 0x0028 (size: 0x8)
    uint8_t id;         // 0x0030 (size: 0x1)
    uint8_t red;        // 0x0031 (size: 0x1)
    uint8_t green;      // 0x0032 (size: 0x1)
    uint8_t blue;       // 0x0033 (size: 0x1)
}; // Size: 0x34

#endif
