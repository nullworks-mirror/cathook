/*
 * RootWindow.h
 *
 *  Created on: Jan 25, 2017
 *      Author: nullifiedcat
 */

#ifndef ROOTWINDOW_H_
#define ROOTWINDOW_H_
#undef RootWindow

#include "menu/CBaseWindow.h"

class RootWindow : public CBaseWindow {
public:
	RootWindow();

	void Setup();

	inline virtual void MoveChildren() override {};
};

#endif /* ROOTWINDOW_H_ */
