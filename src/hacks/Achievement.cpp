/*
 * Achievement.cpp
 *
 *  Created on: Jan 20, 2017
 *      Author: nullifiedcat
 */

#include "Achievement.h"
#include "../common.h"
#include "../sdk.h"

namespace hacks { namespace tf2 { namespace achievement {

CatVar safety(CV_SWITCH, "achievement_safety", "1", "Achievement commands safety switch");

void Lock() {
	if (safety) {
		ConColorMsg({ 255, 0, 0, 255}, "Switch " CON_PREFIX "achievement_safety to 0 before using any achievement commands!\n");
		return;
	}
	interfaces::stats->RequestCurrentStats();
	for (int i = 0; i < interfaces::achievements->GetAchievementCount(); i++) {
		interfaces::stats->ClearAchievement(interfaces::achievements->GetAchievementByIndex(i)->GetName());
	}
	interfaces::stats->StoreStats();
	interfaces::stats->RequestCurrentStats();
}

void Unlock() {
	if (safety) {
		ConColorMsg({ 255, 0, 0, 255}, "Switch " CON_PREFIX "achievement_safety to 0 before using any achievement commands!\n");
		return;
	}
	for (int i = 0; i < interfaces::achievements->GetAchievementCount(); i++) {
		interfaces::achievements->AwardAchievement(interfaces::achievements->GetAchievementByIndex(i)->GetAchievementID());
	}
}

CatCommand lock("achievement_lock", "Lock all achievements", Lock);
CatCommand unlock("achievement_unlock", "Unlock all achievements", Unlock);

}}}
