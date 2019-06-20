/*  This file is part of Cathook.

    Cathook is free software: you can redistribute this file and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Cathook is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Cathook.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "common.hpp"
#include "ChatCommands.hpp"
#include "MiscTemporary.hpp"

static settings::Boolean enabled("chatcommands.enabled", "false");

struct ChatCommand
{
    std::string prefix;
    std::vector<std::string> commands;
};

std::vector<ChatCommand> commands;

namespace hacks::shared::ChatCommands
{
void handleChatMessage(std::string message, int userid)
{
    if (!enabled)
        return;
    logging::Info("%s, %d", message.c_str(), userid);

    std::string prefix;
    std::string::size_type space_pos = message.find_first_of(" ");
    if (space_pos == message.npos)
        prefix = message;
    else
        prefix = message.substr(0, space_pos);
    logging::Info("Prefix: %s", prefix.c_str());

    for (auto var : commands)
    {
        logging::Info("%s : %s", var.prefix.c_str(), prefix.c_str());
        if (prefix != var.prefix)
            continue;
        logging::Info("MATCH!");
        for (auto cmd : var.commands)
            g_IEngine->ClientCmd_Unrestricted(cmd.c_str());
        break;
    }
}

static CatCommand chatcommands_add("chatcommands_add", "chatcommands_add <chat command> <command>", [](const CCommand &args) {
    if (args.ArgC() != 3)
    {
        g_ICvar->ConsoleColorPrintf(Color(*print_r, *print_g, *print_b, 255), "usage: chatcommands_add <chat command> <command>\n");
        return;
    }
    ChatCommand *cmd    = nullptr;
    std::string prefix  = args.Arg(1);
    std::string command = args.Arg(2);

    for (auto &var : commands)
    {
        if (var.prefix == prefix)
        {
            cmd = &var;
            break;
        }
    }

    if (!cmd)
    {
        commands.push_back({});
        cmd = &commands.back();
    }

    logging::Info("%s: %s", prefix.c_str(), command.c_str());

    cmd->prefix = std::move(prefix);
    cmd->commands.push_back(std::move(command));
});

} // namespace hacks::shared::ChatCommands
