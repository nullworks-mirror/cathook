/*
 * utfccp_commands.cpp
 *
 *  Created on: Jul 1, 2017
 *      Author: nullifiedcat
 */

#include "utfccp_commands.hpp"
#include "utfccp.hpp"
#include "common.h"

CatCommand utfccp_encrypt("utfccp_encrypt", "Encrypt a message", [](const CCommand& args) {
	logging::Info("%s", utfccp::encrypt(std::string(args.ArgS())).c_str());
});

CatCommand utfccp_decrypt("utfccp_decrypt", "Decrypt a message", [](const CCommand& args) {
	if (utfccp::validate(std::string(args.ArgS()))) {
		logging::Info("%s", utfccp::decrypt(std::string(args.ArgS())).c_str());
	} else {
		logging::Info("Invalid input data!");
	}
});
