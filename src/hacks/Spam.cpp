/*
 * Spam.cpp
 *
 *  Created on: Jan 21, 2017
 *      Author: nullifiedcat
 */

#include "Spam.h"
#include "../common.h"
#include "../sdk.h"
#include <pwd.h>

namespace hacks { namespace shared { namespace spam {

CatVar enabled(CV_SWITCH, "spam", "0", "Chat spam", "Enable Spam");
CatVar filename(CV_STRING, "spam_file", "spam.txt", "Spam file (~/.cathook/...)", "Spam file name. Each line should be no longer than 100 characters, file must be located in ~/.cathook folder");
CatVar newlines(CV_SWITCH, "spam_newlines", "1", "Spam newlines", "If enabled, several newlines will be added before each message");
CatCommand reload("spam_reload", "Reload spam file", Reload);

int current_index { 0 };
float last_spam { 0.0f };
TextFile file {};

void CreateMove() {
	if (!enabled) return;
	if (last_spam > g_GlobalVars->curtime) last_spam = 0.0f;
	if (!file.LineCount()) return;
	if (g_GlobalVars->curtime - last_spam > 0.8f) {
		if (chat_stack::stack.empty()) {
			if (current_index >= file.LineCount()) current_index = 0;
			std::string spamString = (newlines ? format("\n\n\n\n\n\n\n\n\n\n\n\n", file.Line(current_index)) : file.Line(current_index));
			ReplaceString(spamString, "\\n", "\n");
			chat_stack::stack.push(spamString);
			current_index++;
		}
	}
}

void Reset() {
	last_spam = 0.0f;
}

void Reload() {
	file.Load(std::string(filename.GetString()));
}

}}}
