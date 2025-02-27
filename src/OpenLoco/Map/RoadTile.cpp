#include "../CompanyManager.h"
#include "../GameCommands/GameCommands.h"
#include "../Interop/Interop.hpp"
#include "../Objects/RoadObject.h"
#include "Tile.h"
#include "TileManager.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Map
{
    static loco_global<uint32_t, 0x00525FBC> _525FBC;

    // 0x00477FC2
    bool RoadElement::update(const Map::Pos2& loc)
    {
        if (owner() == CompanyId::neutral || isPlayerCompany(owner()))
            return true;

        if (!(_525FBC & (1 << roadObjectId())))
            return true;

        if (sequenceIndex())
            return true;

        if (hasUnkBit4() || hasLevelCrossing() || mods())
            return true;

        if (isGhost() || isFlag5())
            return true;

        if (hasStationElement())
            return true;

        // Verify there are no other conflicting tile elements on the current tile either.
        // This probably duplicates the above series of checks as well?
        auto tile = TileManager::get(loc);
        for (auto& el : tile)
        {
            auto* roadEl = el.asRoad();
            if (roadEl == nullptr)
                continue;

            if (roadEl->baseZ() != baseZ())
                continue;

            if (roadEl->owner() == CompanyId::neutral || isPlayerCompany(roadEl->owner()))
                continue;

            if (!(_525FBC & (1 << roadEl->roadObjectId())))
                continue;

            if (roadEl->sequenceIndex())
                return true;

            if (hasUnkBit4() || hasLevelCrossing() || mods())
                return true;

            if (roadEl->isGhost() || roadEl->isFlag5())
                return true;

            if (roadEl->hasStationElement())
                return true;
        }

        CompanyId_t backup = CompanyManager::updatingCompanyId();
        CompanyManager::updatingCompanyId(owner());

        GameCommands::RoadRemovalArgs args;
        args.pos = Map::Pos3(loc.x, loc.y, baseZ() * 4);
        args.unkDirection = unkDirection();
        args.roadId = roadId();
        args.sequenceIndex = sequenceIndex();
        args.objectId = roadObjectId();
        GameCommands::doCommand(args, GameCommands::Flags::apply);

        CompanyManager::updatingCompanyId(backup);

        return false;
    }
}
