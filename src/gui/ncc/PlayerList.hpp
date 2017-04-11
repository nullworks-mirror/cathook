/*
 * PlayerList.hpp
 *
 *  Created on: Apr 11, 2017
 *      Author: nullifiedcat
 */

#ifndef PLAYERLIST_HPP_
#define PLAYERLIST_HPP_

#include "../CBaseContainer.h"

namespace menu { namespace ncc {

class PlayerList : public CBaseContainer {
public:
	PlayerList();

	virtual void OnKeyPress(ButtonCode_t key, bool repeat) override;
	virtual void Draw(int x, int y) override;
	virtual void Update() override;
	virtual void MoveChildren() override;
};

}}

#endif /* PLAYERLIST_HPP_ */
