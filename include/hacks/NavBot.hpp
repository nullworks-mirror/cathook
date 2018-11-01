#include "common.hpp"

namespace hacks::shared::NavBot
{

// Main Functions
void Init(bool from_LevelInit);

// Helper
bool CanPath();
bool HasLowHealth();
bool HasLowAmmo();
CachedEntity *nearestHealth();
CachedEntity *nearestAmmo();
std::pair<CachedEntity *, int> nearestEnemy();
CachedEntity *nearestTeleporter();
Vector GetClosestValidByDist(CachedEntity *ent, int idx, float mindist, float maxdist, bool near);
CNavArea *GetNavArea(Vector loc);
void UpdateSlot();
void Jump();

// Path
bool NavToSniperSpot(int priority);
bool NavToNearestEnemy();
bool NavToBacktrackTick(int priority);

} // namespace hacks::shared::NavBot
