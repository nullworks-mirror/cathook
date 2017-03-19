/*
 * ipc.cpp
 *
 *  Created on: Mar 19, 2017
 *      Author: nullifiedcat
 */

#include "ipc.h"

#include "common.h"

namespace ipc {

void CommandCallback(cat_ipc::command_s& command, void* payload) {
	if (!strcmp("exec", (const char*)command.cmd_data) && payload) {
		hack::command_stack().push(std::string((const char*)payload));
	}
}

CatCommand connect("ipc_connect", "Connect to IPC server", []() {
	if (peer) {
		logging::Info("Already connected!");
		return;
	}
	peer = new peer_t(std::string(server_name.GetString()), false, false);
	peer->Connect();
	peer->SetCallback(CommandCallback);
	StoreClientData();
});
CatCommand disconnect("ipc_disconnect", "Disconnect from IPC server", []() {
	if (peer) delete peer;
	peer = nullptr;
});
CatCommand exec("ipc_exec", "Execute command (first argument = bot ID)", [](const CCommand& args) {
	unsigned target_id = atoi(args.Arg(1));
	std::string command = std::string(args.ArgS());
	command = command.substr(command.find(' ', 0) + 1);
	peer->SendMessage("exec", (1 << target_id), command.c_str(), command.length() + 1);
});
CatCommand exec_all("ipc_exec_all", "Execute command (on every peer)", [](const CCommand& args) {
	peer->SendMessage("exec", 0, args.ArgS(), strlen(args.ArgS()) + 1);
});
CatVar server_name(CV_STRING, "ipc_server", "cathook_ipc_server", "IPC server name");

peer_t* peer { nullptr };

void StoreClientData() {
	peer_t::MutexLock lock;
	user_data_s& data = peer->memory->peer_user_data[peer->client_id];
	data.friendid = g_ISteamUser->GetSteamID().GetAccountID();
	strncpy(data.name, g_ISteamFriends->GetPersonaName(), sizeof(data.name));
}

}
