/*
  Created by Jenny White on 29.04.18.
  Copyright (c) 2018 nullworks. All rights reserved.
*/

#include "HookedMethods.hpp"

namespace hooked_methods
{

DEFINE_HOOKED_METHOD(PaintTraverse, void, vgui::IPanel *this_,
                     unsigned int panel, bool force, bool allow_force)
{
    static bool textures_loaded      = false;
    static unsigned long panel_focus = 0;
    static unsigned long panel_scope = 0;
    static unsigned long panel_top   = 0;
    static bool cur, draw_flag = false;
    static bool call_default       = true;
    static ConVar *software_cursor = g_ICvar->FindVar("cl_software_cursor");
    static const char *name;
    static std::string name_s, name_stripped, reason_stripped;

#if ENABLE_VISUALS
    if (!textures_loaded)
    {
        textures_loaded = true;
#ifndef FEATURE_RADAR_DISABLED
        hacks::tf::radar::Init();
#endif
    }
#endif
    if (pure_bypass)
    {
        if (!pure_addr)
        {
            pure_addr = *reinterpret_cast<void ***>(
                    gSignatures.GetEngineSignature(
                            "A1 ? ? ? ? 85 C0 74 ? C7 44 24 ? ? ? ? ? 89 04 24") +
                    1);
        }
        if (*pure_addr)
            pure_orig = *pure_addr;
        *pure_addr    = (void *) 0;
    }
    else if (pure_orig)
    {
        *pure_addr = pure_orig;
        pure_orig  = (void *) 0;
    }
    call_default = true;
    if (cathook && panel_scope && no_zoom && vp == panel_scope)
        call_default = false;

    if (software_cursor_mode)
    {
        cur = software_cursor->GetBool();
        switch ((int) software_cursor_mode)
        {
        case 1:
            if (!software_cursor->GetBool())
                software_cursor->SetValue(1);
            break;
        case 2:
            if (software_cursor->GetBool())
                software_cursor->SetValue(0);
            break;
#if ENABLE_GUI
/*
        case 3:
            if (cur != g_pGUI->Visible()) {
                software_cursor->SetValue(g_pGUI->Visible());
            }
            break;
        case 4:
            if (cur == g_pGUI->Visible()) {
                software_cursor->SetValue(!g_pGUI->Visible());
            }
*/
#endif
        }
    }

    if (call_default)
        original::PaintTraverse(this_, panel, force, allow_force);
    // To avoid threading problems.

    PROF_SECTION(PT_total);

    if (vp == panel_top)
        draw_flag = true;
    if (!cathook)
        return;

    if (!panel_top)
    {
        name = g_IPanel->GetName(vp);
        if (strlen(name) > 4)
        {
            if (name[0] == 'M' && name[3] == 'S')
            {
                panel_top = vp;
            }
        }
    }
    if (!panel_focus)
    {
        name = g_IPanel->GetName(vp);
        if (strlen(name) > 5)
        {
            if (name[0] == 'F' && name[5] == 'O')
            {
                panel_focus = vp;
            }
        }
    }
    if (!panel_scope)
    {
        name = g_IPanel->GetName(vp);
        if (!strcmp(name, "HudScope"))
        {
            panel_scope = vp;
        }
    }
    if (!g_IEngine->IsInGame())
    {
        g_Settings.bInvalid = true;
    }

    if (vp != panel_focus)
        return;
    g_IPanel->SetTopmostPopup(panel_focus, true);
    if (!draw_flag)
        return;
    draw_flag = false;

    if (disable_visuals)
        return;

    if (clean_screenshots && g_IEngine->IsTakingScreenshot())
        return;
#if ENABLE_GUI
    g_pGUI->Update();
#endif
    PROF_SECTION(PT_active);
    draw::UpdateWTS();
}
}