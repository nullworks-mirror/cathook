#include "common.hpp"
#include <hacks/Aimbot.hpp>
namespace hacks::shared::lightesp
{
#if ENABLE_VISUALS
void run();
void draw();
rgba_t LightESPColor(CachedEntity *ent);
#endif
} // namespace hacks::shared::lightesp
