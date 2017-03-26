/*
 * Item.hpp
 *
 *  Created on: Mar 26, 2017
 *      Author: nullifiedcat
 */

#ifndef ITEM_HPP_
#define ITEM_HPP_

#include "../CBaseWidget.h"

namespace menu { namespace ncc {

class Item : public CBaseWidget {
public:
	Item();

	virtual void Draw(int x, int y) override;
};

}}

#endif /* ITEM_HPP_ */
