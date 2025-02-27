#pragma once

#include "Map/Map.hpp"
#include "Types.hpp"
#include <vector>

namespace OpenLoco::Map::TrackData
{
#pragma pack(push, 1)
    struct MoveInfo
    {
        Map::Pos3 loc; // 0x00
        uint8_t yaw;   // 0x06
        uint8_t pitch; // 0x07
    };
#pragma pack(pop)
    static_assert(sizeof(MoveInfo) == 0x8);

    const std::vector<MoveInfo> getTrackSubPositon(uint16_t trackAndDirection);
    const std::vector<MoveInfo> getRoadSubPositon(uint16_t trackAndDirection);
}
