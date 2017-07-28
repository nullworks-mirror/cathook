/*
 * chatlog.cpp
 *
 *  Created on: Jul 28, 2017
 *      Author: nullifiedcat
 */

#include "common.h"
#include "init.hpp"

namespace chatlog {

CatVar enabled(CV_SWITCH, "chat_log", "0", "Chat log", "Log chat to file");
CatVar message_template(CV_STRING, "chat_log_template", "[U:1:%u] %n: %m", "Log template", "%u - SteamID\n%n - name\n%m - message");

class RAIILog {
public:
	RAIILog() {
		stream.open("cathook/chat.log", std::ios::out | std::ios::app);
	}
	~RAIILog() {
		stream.close();
	}
	void log(const std::string& msg) {
		if (stream.bad()) {
			logging::Info("[ERROR] RAIILog stream is bad!");
			return;
		}
		stream << msg << "\n";
		stream.flush();
	}
	std::ofstream stream;
};

RAIILog& logger() {
	static RAIILog object {};
	return object;
}

void LogMessage(int eid, std::string message) {
	if (!enabled) {
		return;
	}
	player_info_s info;
	if (not g_IEngine->GetPlayerInfo(eid, &info))
		return;
	std::string name(info.name);
	for (auto& x : name) {
		if (x < 32) x = '*';
	}
	for (auto& x : message) {
		if (x < 32) x = '*';
	}
	std::string msg(message_template.GetString());
	ReplaceString(msg, "%u", std::to_string(info.friendsID));
	ReplaceString(msg, "%n", name);
	ReplaceString(msg, "%m", message);
	logger().log(msg);
}

}
