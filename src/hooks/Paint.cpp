/*
 * Paint.cpp
 *
 *  Created on: Dec 31, 2017
 *      Author: nullifiedcat
 */

#include <hacks/hacklist.hpp>
#include <online/Online.hpp>
#include <settings/Bool.hpp>
#include "common.hpp"
#include "hitrate.hpp"
#include "hack.hpp"

namespace hooked_methods
{

DEFINE_HOOKED_METHOD(Paint, void, IEngineVGui *this_, PaintMode_t mode)
{
    if (!isHackActive())
    {
        return original::Paint(this_, mode);;
    }

    if (!g_IEngine->IsInGame())
        g_Settings.bInvalid = true;

    if (mode & PaintMode_t::PAINT_UIPANELS)
    {
        hitrate::Update();
#if ENABLE_ONLINE
        online::update();
#endif
#if ENABLE_IPC
        static Timer nametimer{};
        if (nametimer.test_and_set(1000 * 10))
        {
            if (ipc::peer)
            {
                ipc::StoreClientData();
            }
        }
        static Timer ipc_timer{};
        if (ipc_timer.test_and_set(1000))
        {
            if (ipc::peer)
            {
                if (ipc::peer->HasCommands())
                {
                    ipc::peer->ProcessCommands();
                }
                ipc::Heartbeat();
                ipc::UpdateTemporaryData();
            }
        }
#endif
        hacks::shared::autojoin::updateSearch();
        if (!hack::command_stack().empty())
        {
            PROF_SECTION(PT_command_stack);
            std::lock_guard<std::mutex> guard(hack::command_stack_mutex);
            while (!hack::command_stack().empty())
            {
                // logging::Info("executing %s",
                //              hack::command_stack().top().c_str());
                g_IEngine->ClientCmd_Unrestricted(hack::command_stack().top().c_str());
                hack::command_stack().pop();
            }
        }
#if ENABLE_TEXTMODE_STDIN == 1
        static auto last_stdin = std::chrono::system_clock::from_time_t(0);
        auto ms                = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - last_stdin).count();
        if (ms > 500)
        {
            UpdateInput();
            last_stdin = std::chrono::system_clock::now();
        }
#endif
#if ENABLE_VISUALS
        render_cheat_visuals();
#endif
        // Call all paint functions
        EC::run(EC::Paint);
    }

    return original::Paint(this_, mode);
}
} // namespace hooked_methods
