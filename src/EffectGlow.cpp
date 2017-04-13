/*
 * EffectGlow.cpp
 *
 *  Created on: Apr 13, 2017
 *      Author: nullifiedcat
 */

#include "common.h"
#include "EffectGlow.hpp"

// FIXME move to sdk
CScreenSpaceEffectRegistration *CScreenSpaceEffectRegistration::s_pHead = NULL;
IScreenSpaceEffectManager* g_pScreenSpaceEffects = nullptr;
CScreenSpaceEffectRegistration** g_ppScreenSpaceRegistrationHead = nullptr;
CScreenSpaceEffectRegistration::CScreenSpaceEffectRegistration( const char *pName, IScreenSpaceEffect *pEffect )
{
	logging::Info("Creating new effect '%s', head: 0x%08x", pName, *g_ppScreenSpaceRegistrationHead);
	m_pEffectName = pName;
	m_pEffect = pEffect;
	m_pNext = *g_ppScreenSpaceRegistrationHead;
	*g_ppScreenSpaceRegistrationHead = this;
	logging::Info("New head: 0x%08x", *g_ppScreenSpaceRegistrationHead);
}

void EffectGlow::Render(int x, int y, int w, int h) {

}

EffectGlow g_EffectGlow;
CScreenSpaceEffectRegistration* g_pEffectGlow = nullptr;
