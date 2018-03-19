/*
 * Radar.hpp
 *
 *  Created on: Mar 28, 2017
 *      Author: nullifiedcat
 */

#ifndef RADAR_HPP_
#define RADAR_HPP_

#include "../CBaseWidget.h"

namespace menu { namespace ncc {

class Radar : public CBaseWidget {
public:
	Radar();
	virtual void Update() override;
	virtual std::pair<int, int> GetSize() override;
	//virtual void Draw() override;
};

}}

#endif /* RADAR_HPP_ */
