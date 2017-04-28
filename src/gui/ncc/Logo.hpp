/*
 * Logo.hpp
 *
 *  Created on: Apr 28, 2017
 *      Author: nullifiedcat
 */

#ifndef LOGO_HPP_
#define LOGO_HPP_

#include "Menu.hpp"

extern unsigned char _binary_logo_start;

namespace menu { namespace ncc {

class Logo : public CBaseWidget {
public:
	Logo();
	virtual bool AlwaysVisible() override;
	virtual void Draw(int x, int y) override;
	virtual void Update() override;
	Texture texture;
};

}}

#endif /* LOGO_HPP_ */
