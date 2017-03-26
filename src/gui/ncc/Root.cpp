/*
 * Root.cpp
 *
 *  Created on: Mar 26, 2017
 *      Author: nullifiedcat
 */

#include "Root.hpp"
#include "../../common.h"

namespace menu { namespace ncc {

Root::Root() : CBaseWindow("root_nullcore", nullptr) {
	SetMaxSize(draw::width, draw::height);
}

void Root::Draw(int x, int y) {
	CBaseContainer::Draw(x, y);
}

void Root::Setup() {
	AddChild(&menu::ncc::MainList());
	menu::ncc::MainList().SetOffset(500, 500);
}

}}
