/*
 * Walkbot.cpp
 *
 *  Created on: Jul 23, 2017
 *      Author: nullifiedcat
 */

#include "../common.h"

namespace hacks { namespace shared { namespace walkbot {

enum ENodeFlags {
	NF_GOOD = (1 << 0),
	NF_DUCK = (1 << 1),
	NF_JUMP = (1 << 2)
};

struct walkbot_node_s {
	float x { 0.0f }; // 4
	float y { 0.0f }; // 8
	float z { 0.0f }; // 12
	int flags { 0 }; // 16
	int prev { 0 }; // 20
	int next { 0 }; // 24
}; // 24

enum EWalkbotState {
	WB_DISABLED,
	WB_RECORDING,
	WB_EDITING,
	WB_REPLAYING
};

EWalkbotState state { WB_DISABLED };

CatVar pause_recording(CV_SWITCH, "wb_recording_paused", "0", "Pause recording", "Use BindToggle with this");
CatVar draw_info(CV_SWITCH, "wb_info", "1", "Walkbot info");
CatVar draw_path(CV_SWITCH, "wb_path", "1", "Walkbot path");

void Initialize() {
}

void Draw() {
	if (state == WB_DISABLED) return;
	switch (state) {
	case WB_RECORDING: {

	}
	case WB_REPLAYING: {

	}
	}
}

void Move() {
	if (state == WB_DISABLED) return;

}

}}}
