/*
 * Walkbot.cpp
 *
 *  Created on: Jul 23, 2017
 *      Author: nullifiedcat
 */

#include "../common.h"

namespace hacks { namespace shared { namespace walkbot {

using index_t = unsigned;

constexpr index_t INVALID_NODE = unsigned(-1);
constexpr size_t MAX_CONNECTIONS = 4;

enum ENodeFlags {
	NF_GOOD = (1 << 0),
	NF_DUCK = (1 << 1),
	NF_JUMP = (1 << 2)
};

enum EWalkbotState {
	WB_DISABLED,
	WB_RECORDING,
	WB_EDITING,
	WB_REPLAYING
};

struct walkbot_header_s {
	unsigned version { 1 };
	unsigned node_count { 0 };
};

struct walkbot_node_s {
	struct {
		float x { 0 }; // 4
		float y { 0 }; // 8
		float z { 0 }; // 12
	};
	unsigned flags { 0 }; // 16
	size_t connection_count { 0 }; // 20
	index_t connections[MAX_CONNECTIONS]; // 36
	index_t preferred { INVALID_NODE }; // 40

	Vector& xyz() {
		return *reinterpret_cast<Vector*>(&x);
	}

	void link(index_t node) {
		if (connection_count == MAX_CONNECTIONS) {
			logging::Info("[wb] Too many connections! Node at (%.2f %.2f %.2f)", x, y, z);
			return;
		}
		connections[connection_count++] = node;
	}

	void unlink(index_t node) {
		for (size_t i = 0; i < connection_count; i++) {
			if (connections[i] == node) {
				connections[i] = connections[connection_count];
				connections[connection_count--] = INVALID_NODE;
			}
		}
	}
}; // 40

namespace state {

// A vector containing all loaded nodes, used in both recording and replaying
std::vector<walkbot_node_s> nodes {};

// Target node when replaying, selected node when editing, last node when recording
index_t active_node { INVALID_NODE };

// Last reached node when replaying
index_t last_node { INVALID_NODE };

// Node closest to your crosshair when editing
index_t closest_node { INVALID_NODE };

// Global state
EWalkbotState state { WB_DISABLED };

// g_pUserCmd->buttons state when last node was recorded
int last_node_buttons { 0 };

// Set to true when bot is moving to nearest node after dying/losing its active node
bool recovery { true };

// A little bit too expensive function, finds next free node or creates one if no free slots exist
index_t free_node() {
	for (index_t i = 0; i < nodes.size(); i++) {
		if (not (nodes[i].flags & NF_GOOD))
			return i;
	}

	nodes.emplace_back();
	return nodes.size() - 1;
}

bool node_good(index_t node) {
	return node != INVALID_NODE && node < nodes.size() && (nodes[node].flags & NF_GOOD);
}

}

void DeleteNode(index_t node) {
	if (not state::node_good(node))
		return;
	logging::Info("[wb] Deleting node %u", node);
	auto& n = state::nodes[node];
	for (size_t i = 0; i < n.connection_count && i < MAX_CONNECTIONS; i++) {
		if (state::node_good(n.connections[i])) {
			state::nodes[n.connections[i]].unlink(state::closest_node);
		}
	}
	memset(&n, 0, sizeof(walkbot_node_s));
}

index_t CreateNode(const Vector& xyz) {
	index_t node = state::free_node();
	logging::Info("[wb] Creating node %u at (%.2f %.2f %.2f)", node, xyz.x, xyz.y, xyz.z);
	auto& n = state::nodes[node];
	n.xyz() = xyz;
	n.preferred = INVALID_NODE;
	n.connection_count = 0;
	n.flags |= NF_GOOD;
	return node;
}

CatVar active_recording(CV_SWITCH, "wb_recording", "0", "Do recording", "Use BindToggle with this");
CatVar draw_info(CV_SWITCH, "wb_info", "1", "Walkbot info");
CatVar draw_path(CV_SWITCH, "wb_path", "1", "Walkbot path");
CatVar draw_nodes(CV_SWITCH, "wb_nodes", "1", "Walkbot nodes");
CatVar draw_indices(CV_SWITCH, "wb_indices", "1", "Node indices");
CatVar spawn_distance(CV_FLOAT, "wb_node_spawn_distance", "32", "Node spawn distance");
CatVar max_distance(CV_FLOAT, "wb_replay_max_distance", "80", "Max distance to node when replaying");
CatVar reach_distance(CV_FLOAT, "wb_replay_reach_distance", "16", "Distance where bot can be considered 'stepping' on the node");

CatCommand c_start_recording("wb_record", "Start recording", []() { state::state = WB_RECORDING; });
CatCommand c_start_editing("wb_edit", "Start editing", []() { state::state = WB_EDITING; });
CatCommand c_start_replaying("wb_replay", "Start replaying", []() { state::state = WB_REPLAYING; });
CatCommand c_exit("wb_exit", "Exit", []() { state::state = WB_DISABLED; });

// Selects closest node, clears selection if node is selected
CatCommand c_select_node("wb_select", "Select node", []() {
	if (state::active_node == state::closest_node) {
		state::active_node = INVALID_NODE;
	} else {
		state::active_node = state::closest_node;
	}
});
// Deletes closest node and its connections
CatCommand c_delete_node("wb_delete", "Delete node", []() {
	DeleteNode(state::closest_node);
});
// Creates a new node under your feet and connects it to closest node to your crosshair
CatCommand c_create_node("wb_create", "Create node", []() {
	index_t node = CreateNode(g_pLocalPlayer->v_Origin);
	auto& n = state::nodes[node];
	if (g_pUserCmd->buttons & IN_DUCK)
		n.flags |= NF_DUCK;
	if (state::node_good(state::closest_node)) {
		auto& c = state::nodes[state::closest_node];
		n.link(state::closest_node);
		c.link(node);
		logging::Info("[wb] Node %u linked to node %u at (%.2f %.2f %.2f)", node, state::closest_node, c.x, c.y, c.z);
	}
});
// Connects selected node to closest one
CatCommand c_connect_node("wb_connect", "Connect node", []() {
	if (not (state::node_good(state::active_node) and state::node_good(state::closest_node)))
		return;
	// Don't link a node to itself, idiot
	if (state::active_node == state::closest_node)
		return;

	auto& a = state::nodes[state::active_node];
	auto& b = state::nodes[state::closest_node];

	a.link(state::closest_node);
	b.link(state::active_node);
});
// Updates duck flag on region of nodes (selected to closest)
// Updates a single closest node if no node is selected
CatCommand c_update_duck("wb_duck", "Update duck flags", []() {
	index_t a = state::active_node;
	index_t b = state::closest_node;

	if (not (state::node_good(a) and state::node_good(b)))
		return;

	index_t current = state::closest_node;

	do {
		auto& n = state::nodes[current];
		if (g_pUserCmd->buttons & IN_DUCK)
			n.flags |= NF_DUCK;
		else
			n.flags &= ~NF_DUCK;
		if (n.connection_count > 2) {
			logging::Info("[wb] More than 2 connections on a node - instructions unclear, got my duck stuck in 'if' block");
			return;
		}
		bool found_next = false;
		for (size_t i = 0; i < 2; i++) {
			if (n.connections[i] != current) {
				current = n.connections[i];
				found_next = true;
				break;
			}
		}
		if (not found_next) {
			logging::Info("[wb] Dead end? Can't find next node after %u", current);
			break;
		}
	} while (state::node_good(current) and (current != a));
});
// Toggles jump flag on closest node
CatCommand c_update_jump("wb_jump", "Toggle jump flag", []() {
	if (not state::node_good(state::closest_node))
		return;

	auto& n = state::nodes[state::closest_node];

	if (n.flags & NF_JUMP)
		n.flags &= ~NF_JUMP;
	else
		n.flags |= NF_JUMP;
});
// Sets the closest node as preferred path for the selected node (or disable it)
CatCommand c_set_preferred("wb_prefer", "Set preferred node", []() {
	index_t a = state::active_node;
	index_t b = state::closest_node;

	if (not (state::node_good(a) and state::node_good(b)))
		return;

	auto& n = state::nodes[a];
	if (n.preferred == b) {
		n.preferred = INVALID_NODE;
		return;
	}

	bool found = false;
	for (size_t i = 0; i < n.connection_count; i++) {
		if (n.connections[i] == b) {
			if (found) {
				logging::Info("[wb] WARNING!!! Duplicate connection to %u on node %u!!!", a, b);
			}
			found = true;
		}
	}
	n.preferred = b;
});
// Displays all info about closest node
CatCommand c_info("wb_info", "Show info", []() {
	if (not state::node_good(state::closest_node))
		return;

	auto& n = state::nodes[state::closest_node];

	logging::Info("[wb] Info about node %u", state::closest_node);
	logging::Info("[wb] FLAGS: Duck: %d, Jump: %d, Raw: %u", n.flags & NF_DUCK, n.flags & NF_JUMP, n.flags);
	logging::Info("[wb] X: %.2f | Y: %.2f | Z: %.2f", n.x, n.y, n.z);
	logging::Info("[wb] CONNECTIONS: %d/%d", n.connection_count, MAX_CONNECTIONS);
	for (size_t i = 0; i < n.connection_count; i++) {
		logging::Info("[wb] %u <-> %u", state::closest_node, n.connections[i]);
		auto& c = state::nodes[n.connections[i]];
		bool found = false;
		for (size_t j = 0; j < c.connection_count; j++) {
			if (c.connections[j] == state::closest_node) {
				if (found) {
					logging::Info("[wb] DUPLICATE CONNECTION: %u <-> %u", i, state::closest_node);
				}
				found = true;
			}
		}
		if (not found) {
			logging::Info("[wb] CONNECTION IS SINGLE-DIRECTIONAL (BROKEN)! (%u)", i);
		}
	}
});
// Deletes a whole region of nodes
// Deletes a single closest node if no node is selected
CatCommand c_delete_region("wb_delete_region", "Delete region of nodes", []() {
	index_t a = state::active_node;
	index_t b = state::closest_node;

	if (not (state::node_good(a) and state::node_good(b)))
		return;

	index_t current = state::closest_node;
	index_t next = INVALID_NODE;

	do {
		auto& n = state::nodes[current];

		if (n.connection_count > 2) {
			logging::Info("[wb] More than 2 connections on a node! Quitting.");
			return;
		}
		bool found_next = false;
		for (size_t i = 0; i < 2; i++) {
			if (n.connections[i] != current) {
				next = n.connections[i];
				found_next = true;
			}
		}
		DeleteNode(current);
		current = next;
		if (not found_next) {
			logging::Info("[wb] Dead end? Can't find next node after %u", current);
			break;
		}
	} while (state::node_good(current) and (current != a));
});

void Initialize() {
}

void UpdateClosestNode() {
	float n_fov = 360.0f;
	index_t n_idx = INVALID_NODE;

	for (index_t i = 0; i < state::nodes.size(); i++) {
		const auto& node = state::nodes[i];

		if (not node.flags & NF_GOOD)
			continue;

		float fov = GetFov(g_pLocalPlayer->v_OrigViewangles, g_pLocalPlayer->v_Eye, node.xyz());
		if (fov < n_fov) {
			n_fov = fov;
			n_idx = i;
		}
	}

	// Don't select a node if you don't even look at it
	if (n_fov < 10)
		state::closest_node = n_idx;
	else
		state::closest_node = INVALID_NODE;
}

// Finds nearest node by position, not FOV
// Not to be confused with FindClosestNode
index_t FindNearestNode() {
	index_t r_node { INVALID_NODE };
	float r_dist { 65536.0f };

	for (index_t i = 0; i < state::nodes.size(); i++) {
		if (state::node_good(i)) {
			auto& n = state::nodes[i];
			float dist = g_pLocalPlayer->v_Origin.DistTo(n.xyz());
			if (dist < r_dist) {
				r_dist = dist;
				r_node = i;
			}
		}
	}

	return r_node;
}

index_t SelectNextNode() {
	 if (not state::node_good(state::active_node)) {
		 return FindNearestNode();
	 }
	 auto& n = state::nodes[state::active_node];
	 if (n.connection_count > 2) {
		 if (state::node_good(n.preferred)) {
			 return n.preferred;
		 } else {
			 std::vector<index_t> chance {};
			 for (index_t i = 0; i < n.connection_count && i < MAX_CONNECTIONS; i++) {
				 if (n.connections[i] != state::active_node && state::node_good(n.connections[i])) {
					 chance.push_back(n.connections[i]);
				 }
			 }
			 if (not chance.empty()) {
				 return chance.at(rand() % chance.size());
			 } else {
				 return INVALID_NODE;
			 }
		 }
	 }
	 for (index_t i = 0; i < n.connection_count && i < MAX_CONNECTIONS; i++) {
		 if (n.connections[i] != state::active_node && state::node_good(n.connections[i])) {
			 return n.connections[i];
		 }
	 }
	 return INVALID_NODE;
}

void UpdateWalker() {
	 if (not state::node_good(state::active_node)) {
		 state::active_node = FindNearestNode();
		 state::recovery = true;
	 }
	 auto& n = state::nodes[state::active_node];
	 WalkTo(n.xyz());
	 float dist = n.xyz().DistTo(g_pLocalPlayer->v_Origin);
	 if (dist > float(max_distance)) {
		 state::recovery = true;
	 }
	 if (dist < float(reach_distance)) {
		 state::recovery = false;
		 state::last_node = state::active_node;
		 state::active_node = SelectNextNode();
		 logging::Info("[wb] Reached node %u, moving to %u", state::last_node, state::active_node);
		 if (not state::node_good(state::active_node) and not state::recovery) {
			 logging::Info("[wb] FATAL: Next node is bad");
			 state::recovery = true;
		 }
	 }
}

// Draws a single colored connection between 2 nodes
void DrawConnection(index_t a, index_t b) {
	if (not (state::node_good(a) and state::node_good(b)))
		return;

	const auto& a_ = state::nodes[a];
	const auto& b_ = state::nodes[b];

	Vector wts_a, wts_b;
	if (not (draw::WorldToScreen(a_.xyz(), wts_a) and draw::WorldToScreen(b_.xyz(), wts_b)))
		return;

	rgba_t* color = &colors::white;
	if 		((a_.flags & b_.flags) & NF_JUMP) color = &colors::yellow;
	else if ((a_.flags & b_.flags) & NF_DUCK) color = &colors::green;

	if (a_.preferred == b or b_.preferred == a)
		color = &colors::pink;

	drawgl::Line(wts_a.x, wts_a.y, wts_b.x - wts_a.x, wts_b.y - wts_a.y, color->rgba);
}

// Draws a node and its connections
void DrawNode(index_t node, bool draw_back) {
	if (not state::node_good(node))
		return;

	const auto& n = state::nodes[node];

	for (size_t i = 0; i < n.connection_count && i < MAX_CONNECTIONS; i++) {
		index_t connection = n.connections[i];
		// To prevent drawing connections twice in a for loop, we only draw connections to nodes with higher index
		if (not draw_back) {
			if (connection < node)
				continue;
		}
		DrawConnection(node, connection);
	}

	if (draw_nodes) {
		rgba_t* color = &colors::white;
		if 		(n.flags & NF_JUMP) color = &colors::yellow;
		else if (n.flags & NF_DUCK) color = &colors::green;

		Vector wts;
		if (not draw::WorldToScreen(n.xyz(), wts))
			return;

		size_t node_size = 2;
		if (node == state::closest_node)
			node_size = 6;
		if (node == state::active_node)
			color = &colors::red;

		drawgl::FilledRect(wts.x - node_size, wts.y - node_size, 2 * node_size, 2 * node_size, color->rgba);
	}

	if (draw_indices) {
		rgba_t* color = &colors::white;
		if 		(n.flags & NF_JUMP) color = &colors::yellow;
		else if (n.flags & NF_DUCK) color = &colors::green;

		Vector wts;
		if (not draw::WorldToScreen(n.xyz(), wts))
			return;

		FTGL_Draw(std::to_string(node), wts.x, wts.y, fonts::ftgl_ESP, *color);
	}
}

bool ShouldSpawnNode() {
	if (not state::node_good(state::active_node))
		return true;

	bool was_jumping = state::last_node_buttons & IN_JUMP;
	bool is_jumping = g_pUserCmd->buttons & IN_JUMP;

	if (was_jumping != is_jumping and is_jumping)
		return true;

	if ((state::last_node_buttons & IN_DUCK) != (g_pUserCmd->buttons & IN_DUCK))
		return true;

	auto& node = state::nodes[state::active_node];

	if (node.xyz().DistTo(g_pLocalPlayer->v_Origin) > float(spawn_distance)) {
		return true;
	}

	return false;
}

void RecordNode() {
	index_t node = CreateNode(g_pLocalPlayer->v_Origin);
	auto& n = state::nodes[node];
	if (g_pUserCmd->buttons & IN_DUCK)
		n.flags |= NF_DUCK;
	if (g_pUserCmd->buttons & IN_JUMP)
		n.flags |= NF_JUMP;
	if (state::node_good(state::active_node)) {
		auto& c = state::nodes[state::active_node];
		n.link(state::active_node);
		c.link(node);
		logging::Info("[wb] Node %u auto-linked to node %u at (%.2f %.2f %.2f)", node, state::active_node, c.x, c.y, c.z);
	}
	state::last_node_buttons = g_pUserCmd->buttons;
	state::active_node = node;
}

void DrawPath() {
	for (index_t i = 0; i < state::nodes.size(); i++) {
		DrawNode(i, false);
	}
}

void Draw() {
	if (state::state == WB_DISABLED) return;
	switch (state::state) {
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
	if (draw_path)
		DrawPath();
}

void Move() {
	if (state::state == WB_DISABLED) return;
	switch (state::state) {
	case WB_RECORDING: {
		if (active_recording and ShouldSpawnNode()) {
			RecordNode();
		}
	} break;
	case WB_EDITING: {
		UpdateClosestNode();
	} break;
	case WB_REPLAYING: {
		UpdateWalker();
	} break;
	}
}

}}}
