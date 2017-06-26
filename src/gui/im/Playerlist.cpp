/*
 * Playerlist.cpp
 *
 *  Created on: May 21, 2017
 *      Author: nullifiedcat
 */

#include "Playerlist.hpp"
#include "../../common.h"
#include "../../playerlist.hpp"

#include "imgui.h"

namespace menu { namespace im {

void RenderPlayer(int eid) {
	ImGui::PushID(eid);
	player_info_s info;
	bool success = g_IEngine->GetPlayerInfo(eid, &info);
	if (success) {
		//ImGui::PushItemWidth(50);
			ImGui::Text("%d", info.userID);
		//ImGui::PopItemWidth();
		ImGui::SameLine(50);
		//ImGui::PushItemWidth(100);
			ImGui::Text("%u", info.friendsID);
		//ImGui::PopItemWidth();
		ImGui::SameLine(150);

		char safename[32];
		for (int i = 0, j = 0; i < 32; i++) {
			if (info.name[i] == 0) {
				safename[j] = 0;
				break;
			}
			if (info.name[i] == '\n') continue;
			safename[j++] = info.name[i];
		}
		//ImGui::PushItemWidth(250);
			ImGui::Text("%s", safename);
		//ImGui::PopItemWidth();
		ImGui::SameLine(325);

		int iclazz = 0;
		rgba_t bgcolor = colors::empty;
		const char* text = "N/A";
		IF_GAME (IsTF()) {
			iclazz = g_pPlayerResource->GetClass(ENTITY(eid));
			int team = g_pPlayerResource->GetTeam(eid);
			if (eid != g_IEngine->GetLocalPlayer()) {
				if (team == TEAM_RED) {
					bgcolor = colors::red;
				} else if (team == TEAM_BLU) {
					bgcolor = colors::blu;
				}
			}
			if (iclazz && iclazz < 10) {
				text = classes[iclazz - 1].c_str();
			}
		}

		if (bgcolor.a) {
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(bgcolor.r, bgcolor.g, bgcolor.b, bgcolor.a));
		}

		//ImGui::PushItemWidth(150);
			ImGui::Text("%s", text);
		//ImGui::PopItemWidth();
		ImGui::SameLine(400);

		if (bgcolor.a) {
			ImGui::PopStyleColor();
		}
		playerlist::userdata& data = playerlist::AccessData(info.friendsID);
		int& state = *reinterpret_cast<int*>(&data.state);
		bgcolor = playerlist::Color(info.friendsID);
		if (bgcolor.a) {
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(bgcolor.r, bgcolor.g, bgcolor.b, bgcolor.a));
		}
		ImGui::PushItemWidth(120);
		ImGui::Combo("", &state, playerlist::k_pszNames, 5);
		ImGui::PopItemWidth();
		if (bgcolor.a) {
			ImGui::PopStyleColor();
		}
		ImGui::SameLine();
		ImGui::PushItemWidth(200.0f);
		if (ImGui::ColorEdit3("", data.color)) {
			if (!data.color.r && !data.color.b && !data.color.g) data.color = colors::empty;
		}
		ImGui::PopItemWidth();
	}
	ImGui::PopID();
}

void RenderPlayerlist() {
	if (!g_IEngine->IsInGame()) return;
	if (ImGui::Begin("Player List")) {
		ImGui::SetWindowSize(ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 1));
		for (int i = 1; i < 32; i++) {
			RenderPlayer(i);
		}
		ImGui::PopStyleVar();
	}
	ImGui::End();
}

}}
