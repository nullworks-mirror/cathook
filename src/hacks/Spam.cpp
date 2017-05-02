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
static CatEnum spam_enum({"DISABLED", "DEFAULT", "LENNYFACES", "BLANKS", "FROM FILE"});
CatVar spam_source(spam_enum, "spam", "0", "Chat Spam", "Defines source of spam lines. CUSTOM spam file must be set in cat_spam_file and loaded with cat_spam_reload (Use console!)");
CatVar random_order(CV_SWITCH, "spam_random", "0", "Random Order");
CatVar filename(CV_STRING, "spam_file", "spam.txt", "Spam file (~/.cathook/...)", "Spam file name. Each line should be no longer than 100 characters, file must be located in ~/.cathook folder");
CatCommand reload("spam_reload", "Reload spam file", Reload);

int current_index { 0 };
float last_spam { 0.0f };
TextFile file {};

void CreateMove() {
	if (!spam_source) return;
	if (last_spam > g_GlobalVars->curtime) last_spam = 0.0f;
	const std::vector<std::string>* source = nullptr;
	switch ((int)spam_source) {
	case 1:
		source = &builtin_default; break;
	case 2:
		source = &builtin_lennyfaces; break;
	case 3:
		source = &builtin_blanks; break;
	case 4:
		source = &file.lines; break;
	default:
		return;
	}
	if (!source || !source->size()) return;
	if (g_GlobalVars->curtime - last_spam > 0.8f) {
		if (chat_stack::stack.empty()) {
			if (current_index >= source->size()) current_index = 0;
			if (random_order) current_index = rand() % source->size();
			std::string spamString = source->at(current_index);
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

const std::vector<std::string> builtin_default = {
		"cathook - more fun than a ball of yarn!",
		"GNU/Linux is the best OS!",
		"get cathook: discord.gg/7bu3AFw",
		"cathook - free tf2 cheat!"
};
const std::vector<std::string> builtin_lennyfaces = {
		"( ͡° ͜ʖ ͡°)", "( ͡°( ͡° ͜ʖ( ͡° ͜ʖ ͡°)ʖ ͡°) ͡°)", "ʕ•ᴥ•ʔ",
		"(▀̿Ĺ̯▀̿ ̿)", "( ͡°╭͜ʖ╮͡° )", "(ง'̀-'́)ง", "(◕‿◕✿)",
		"༼ つ  ͡° ͜ʖ ͡° ༽つ" };
const std::vector<std::string> builtin_blanks = {
		". \n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n "
};

}}}
