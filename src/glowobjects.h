/*
 * glowobjects.h
 *
 *  Created on: Jan 16, 2017
 *      Author: nullifiedcat
 */

#ifndef GLOWOBJECTS_H_
#define GLOWOBJECTS_H_

#define END_OF_FREE_LIST -1
#define ENTRY_IN_USE -2

#include "common.h"

//template<typename T> class CUtlVector;

extern std::set<int> g_CathookManagedGlowObjects;

struct GlowObjectDefinition_t {
    CBaseHandle m_hEntity;
    Vector m_vGlowColor;
    float m_flGlowAlpha;
    bool m_bRenderWhenOccluded;
    bool m_bRenderWhenUnoccluded;
    int m_nSplitScreenSlot;
    int m_nNextFreeSlot;
};

class CGlowObjectManager {
public:
	int EnableGlow(IClientEntity* entity, int color);
	void DisableGlow(int idx);
	int GlowHandle(IClientEntity* entity) const;
	CUtlVector<GlowObjectDefinition_t> m_GlowObjectDefinitions;
	int m_nFirstFreeSlot;
};

extern CGlowObjectManager* g_GlowObjectManager;

bool CanEntityEvenGlow(int idx);
int GetEntityGlowColor(int idx);
bool ShouldEntityGlow(int idx);

#endif /* GLOWOBJECTS_H_ */
