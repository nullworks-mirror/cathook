/*
 * Walkbot.cpp
 *
 *  Created on: Jul 23, 2017
 *      Author: nullifiedcat
 */

#include "../common.h"

namespace hacks { namespace shared { namespace walkbot {

constexpr unsigned INVALID_NODE = unsigned(-1);

enum ENodeFlags {
	NF_GOOD = (1 << 0),
	NF_DUCK = (1 << 1),
	NF_JUMP = (1 << 2)
};

struct walkbot_header_s {
	unsigned version { 1 };
	unsigned node_count { 0 };
	unsigned first_node { 0 };
};

struct walkbot_node_s {
	float x { 0.0f }; // 4
	float y { 0.0f }; // 8
	float z { 0.0f }; // 12
	unsigned flags { 0 }; // 16
	unsigned prev { 0 }; // 20
	unsigned next { 0 }; // 24
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
		AddSideString("Walkbot: Recording");

	} break;
	case WB_EDITING: {
		AddSideString("Walkbot: Editing");

	} break;
	case WB_REPLAYING: {
		AddSideString("Walkbot: Replaying");

	} break;
	}
}

void Move() {
	if (state == WB_DISABLED) return;

}

}}}
