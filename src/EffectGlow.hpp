/*
 * EffectGlow.hpp
 *
 *  Created on: Apr 13, 2017
 *      Author: nullifiedcat
 */

#ifndef EFFECTGLOW_HPP_
#define EFFECTGLOW_HPP_

#include "common.h"

namespace effect_glow {

class EffectGlow : public IScreenSpaceEffect {
public:
	virtual void Init( );
	inline virtual void Shutdown( ) {};

	inline virtual void SetParameters( KeyValues *params ) {};

	virtual void Render( int x, int y, int w, int h );

	inline virtual void Enable( bool bEnable ) { enabled = bEnable; };
	inline virtual bool IsEnabled( ) { return enabled; };

	void DrawEntity(IClientEntity* entity);
	void DrawToStencil(IClientEntity* entity);
	void DrawToBuffer(IClientEntity* entity);
	int ChamsColor(IClientEntity* entity);
	bool ShouldRenderChams(IClientEntity* entity);
	void RenderChams(int idx);
	void BeginRenderChams();
	void EndRenderChams();
public:
	bool init { false };
	bool drawing { false };
	bool enabled;
	float orig_modulation[3];
	CMaterialReference mat_unlit;
	CMaterialReference mat_unlit_z;
};

extern EffectGlow g_EffectGlow;
extern CScreenSpaceEffectRegistration* g_pEffectGlow;

}

#endif /* EFFECTGLOW_HPP_ */
