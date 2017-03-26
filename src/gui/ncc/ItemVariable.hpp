/*
 * ItemVariable.hpp
 *
 *  Created on: Mar 26, 2017
 *      Author: nullifiedcat
 */

#ifndef ITEMVARIABLE_HPP_
#define ITEMVARIABLE_HPP_

#include "Item.hpp"

class CatVar;

namespace menu { namespace ncc {

class ItemVariable : public Item {
public:
	ItemVariable(CatVar* variable);

	virtual void Draw(int x, int y) override;
public:

};

}}

#endif /* ITEMVARIABLE_HPP_ */
