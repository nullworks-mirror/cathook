/*
 * xb64.hpp
 *
 *  Created on: Jul 1, 2017
 *      Author: nullifiedcat
 */

#pragma once

#include <string>
#include "base64.h"

namespace xb64 {

std::string crappy_checksum(const std::string& in) {
	int s = 0, ss = 0;
	for (int i = 0; i < in.length(); i += 2) {
		s += (int)in[i];
		if (i < in.length() - 1)
			ss += (int)in[i + 1] * (int)in[i + 1];
	}
	return { kBase64Alphabet[s % 64], kBase64Alphabet[ss % 64] };
}

std::string crappy_xorstring(const std::string& in) {
	std::string out;
	for (int i = 0; i < in.length(); i++) {
		// 696969th prime, 13371337th prime
		out.push_back(in[i] ^ ((10522103 * in.length()) + i * 244053389 % 256));
	}
	return out;
}

bool validate(const std::string& in) {
	if (in.length() < 2) return false;
	return crappy_checksum(in.substr(0, in.length() - 2)) == in.substr(in.length() - 2);
}

std::string decrypt(const std::string& in) {
	std::string b64;
	Base64::Decode(in.substr(0, in.length() - 2), &b64);
	return crappy_xorstring(b64);
}

std::string encrypt(const std::string& in) {
	std::string b64;
	Base64::Encode(crappy_xorstring(in), &b64);
	return b64 + crappy_checksum(b64);
}

}
