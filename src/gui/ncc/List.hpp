/*
 * List.hpp
 *
 *  Created on: Mar 26, 2017
 *      Author: nullifiedcat
 */

#ifndef LIST_HPP_
#define LIST_HPP_

#include "../CBaseContainer.h"

class Item;

namespace menu { namespace ncc {

class List : public CBaseContainer {
public:
	List(std::string title);

	virtual void Draw(int x, int y) override;
	virtual void Update() override;
	virtual void MoveChildren() override;
public:
	List* open_sublist;
	const std::string title;
	std::vector<Item*> items;
};

}}

#endif /* LIST_HPP_ */
