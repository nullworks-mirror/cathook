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
CachedEntity *nearestEnemy();
Vector GetClosestValidByDist(CachedEntity *ent, float mindist, float maxdist, bool near);
void UpdateSlot();
void Jump();

// Path
bool NavToSniperSpot(int priority);
bool NavToNearestEnemy();
bool NavToBacktrackTick(int priority);

} // namespace hacks::shared::NavBot
