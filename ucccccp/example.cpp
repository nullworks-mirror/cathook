/*
	EXAMPLE! Usage of UCCCCP in hooks.
	Partly pasted from cathook
	WILL NOT COMPILE/WORK IN THAT WAY! YOU HAVE TO ADAPT THIS CODE TO YOUR CHEAT.
*/

#include "ucccccp.hpp"
#include "cheat.hpp"

// COMMANDS (ripped straight from cathook)

CatCommand ucccccp_encrypt("ucccccp_encrypt", "Encrypt a message", [](const CCommand& args) {
	logging::Info("%s", ucccccp::encrypt(std::string(args.ArgS())).c_str());
});

CatCommand ucccccp_decrypt("ucccccp_decrypt", "Decrypt a message", [](const CCommand& args) {
	if (ucccccp::validate(std::string(args.ArgS()))) {
		logging::Info("%s", ucccccp::decrypt(std::string(args.ArgS())).c_str());
	} else {
		logging::Info("Invalid input data!");
	}
});

// SENDING messages

bool SendNetMsg_hook(void* _this, INetMessage& msg, bool bForceReliable = false, bool bVoice = false) {
	const SendNetMsg_t original = (SendNetMsg_t)hooks::netchannel.GetMethod(offsets::SendNetMsg());
	// net_StringCmd
	if (msg.GetType() == 4 && (newlines_msg || crypt_chat)) {
		std::string str(msg.ToString());
		say_idx = str.find("net_StringCmd: \"say \"");
		say_team_idx = str.find("net_StringCmd: \"say_team \"");
		if (!say_idx || !say_team_idx) {
			offset = say_idx ? 26 : 21;
			bool processed = false;
			if (crypt_chat) {
				std::string msg(str.substr(offset));
				msg = msg.substr(0, msg.length() - 2);
				if (msg.find("!!") == 0) {
					msg = ucccccp::encrypt(msg.substr(2));
					str = str.substr(0, offset) + msg + "\"\"";
					processed = true;
				}
			}
			if (processed) {
				str = str.substr(16, str.length() - 17);
				stringcmd.m_szCommand = str.c_str();
				return original(_this, stringcmd, bForceReliable, bVoice);
			}
		}
	}
	return original(_this, msg, bForceReliable, bVoice);
}

// RECEIVING messages

bool DispatchUserMessage_hook(void* _this, int type, bf_read& buf) {
	static const DispatchUserMessage_t original = (DispatchUserMessage_t)hooks::client.GetMethod(offsets::DispatchUserMessage());
	if (crypt_chat) {
		if (type == 4) {
			int k = 0, j = 0;
			int s = buf.GetNumBytesLeft();
			if (s < 256) {
				data = (char*)alloca(s);
				// Read the message buffer.
				for (i = 0; i < s; i++)
					data[i] = buf.ReadByte();
				j = 0;
				// You can use other containers, I just like string.
				// Message is formatted like that: [garbo]\0[name]\0[message]
				std::string name;
				std::string message;
				for (int i = 0; i < 3; i++) {
					while ((c = data[j++]) && (k < 128)) {
						k++;
						if (i == 1) name.push_back(c);
						if (i == 2) message.push_back(c);
					}
				}
				if (crypt_chat) {
					if (message.find("!!") == 0) {
						// We must make sure that message is actually encrypted message, not just garbo sent by normie
						if (ucccccp::validate(message)) {
							// Decrypt and print to chat!
							PrintChat("%s: %s", name.c_str(), ucccccp::decrypt(message).c_str());
						}
					}
				}
				// Reset buffer
				buf = bf_read(data, s);
				buf.Seek(0);
			}
		}
	}
	// Call the original function
	return original(_this, type, buf);
}


