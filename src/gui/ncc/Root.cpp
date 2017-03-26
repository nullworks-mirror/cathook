/*
 * Root.cpp
 *
 *  Created on: Mar 26, 2017
 *      Author: nullifiedcat
 */

#include "Root.hpp"
#include "../../common.h"

namespace menu { namespace ncc {

Root::Root() : CBaseWindow("root_nullcore", nullptr) {}

void Root::Setup() {
	AddChild(&menu::ncc::MainList());
}

}}
