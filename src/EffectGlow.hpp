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
	inline virtual void Init( ) {};
	inline virtual void Shutdown( ) {};

	inline virtual void SetParameters( KeyValues *params ) {};

	virtual void Render( int x, int y, int w, int h );

	inline virtual void Enable( bool bEnable ) { enabled = bEnable; };
	inline virtual bool IsEnabled( ) { return enabled; };

public:
	bool enabled;
};

extern EffectGlow g_EffectGlow;
extern CScreenSpaceEffectRegistration* g_pEffectGlow;

#endif /* EFFECTGLOW_HPP_ */
