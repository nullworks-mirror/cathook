/*
 * EffectGlow.hpp
 *
 *  Created on: Apr 13, 2017
 *      Author: nullifiedcat
 */

#ifndef EFFECTGLOW_HPP_
#define EFFECTGLOW_HPP_

#include "common.h"

class EffectGlow : public IScreenSpaceEffect {
public:
	virtual void Init( );
	inline virtual void Shutdown( ) {};

	inline virtual void SetParameters( KeyValues *params ) {};

	virtual void Render( int x, int y, int w, int h );

	inline virtual void Enable( bool bEnable ) { enabled = bEnable; };
	inline virtual bool IsEnabled( ) { return enabled; };

	void RenderGlow(int idx);
	void BeginRenderGlow();
	void EndRenderGlow();
public:
	bool init { false };
	bool enabled;
	float orig_modulation[3];
	CTextureReference rt_A;
	CTextureReference rt_B;
	CMaterialReference result_material;
	CMaterialReference glow_material;
	CMaterialReference dev_bloomdadd;
	CMaterialReference dev_halo_add_to_screen;
	CMaterialReference dev_blurfilterx;
	CMaterialReference dev_blurfiltery;
};

extern EffectGlow g_EffectGlow;
extern CScreenSpaceEffectRegistration* g_pEffectGlow;

#endif /* EFFECTGLOW_HPP_ */
