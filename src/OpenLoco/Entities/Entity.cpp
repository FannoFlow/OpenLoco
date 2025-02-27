#include "Entity.h"
#include "../Config.h"
#include "../Graphics/Gfx.h"
#include "../Interop/Interop.hpp"
#include "../ViewportManager.h"
#include <algorithm>

using namespace OpenLoco;
using namespace OpenLoco::Interop;

bool EntityBase::isEmpty() const
{
    return base_type == EntityBaseType::null;
}

// 0x0046FC83
void EntityBase::moveTo(const Map::Pos3& loc)
{
    registers regs;
    regs.ax = loc.x;
    regs.cx = loc.y;
    regs.dx = loc.z;
    regs.esi = X86Pointer(this);
    call(0x0046FC83, regs);
}

// 0x004CBB01
void OpenLoco::EntityBase::invalidateSprite()
{
    Ui::ViewportManager::invalidate(this, ZoomLevel::eighth);
}
