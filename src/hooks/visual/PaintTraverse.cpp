/*
  Created by Jenny White on 29.04.18.
  Copyright (c) 2018 nullworks. All rights reserved.
*/

#include "HookedMethods.hpp"
#include "Radar.hpp"

CatVar clean_screenshots(CV_SWITCH, "clean_screenshots", "1",
                         "Clean screenshots",
                         "Don't draw visuals while taking a screenshot");
CatVar disable_visuals(CV_SWITCH, "no_visuals", "0", "Disable ALL drawing",
                       "Completely hides cathook");
CatVar no_zoom(CV_SWITCH, "no_zoom", "0", "Disable scope",
               "Disables black scope overlay");
static CatVar pure_bypass(CV_SWITCH, "pure_bypass", "0", "Pure Bypass",
                          "Bypass sv_pure");
void *pure_orig  = nullptr;
void **pure_addr = nullptr;

static CatEnum software_cursor_enum({ "KEEP", "ALWAYS", "NEVER", "MENU ON",
                                      "MENU OFF" });
static CatVar
    software_cursor_mode(software_cursor_enum, "software_cursor_mode", "0",
                         "Software cursor",
                         "Try to change this and see what works best for you");
static CatVar no_reportlimit(CV_SWITCH, "no_reportlimit", "0",
                             "no report limit",
                             "Remove playerlist report time limit");
bool replaced = false;
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
    }
#endif
    if (no_reportlimit && !replaced)
    {
        static unsigned char patch[] = { 0xB8, 0x01, 0x00, 0x00, 0x00 };
        static uintptr_t report_addr = gSignatures.GetClientSignature(
            "55 89 E5 57 56 53 81 EC ? ? ? ? 8B 5D ? 8B 7D ? 89 D8");
        if (report_addr)
        {
            uintptr_t topatch = report_addr + 0x75;
            logging::Info("No Report limit: 0x%08x", report_addr);
            Patch((void *) topatch, (void *) patch, sizeof(patch));
            replaced = true;
        }
        else
            report_addr = gSignatures.GetClientSignature(
                "55 89 E5 57 56 53 81 EC ? ? ? ? 8B 5D ? 8B 7D ? 89 D8");
    }
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
    if (cathook && panel_scope && no_zoom && panel == panel_scope)
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

    if (panel == panel_top)
        draw_flag = true;
    if (!cathook)
        return;

    if (!panel_top)
    {
        name = g_IPanel->GetName(panel);
        if (strlen(name) > 4)
        {
            if (name[0] == 'M' && name[3] == 'S')
            {
                panel_top = panel;
            }
        }
    }
    if (!panel_focus)
    {
        name = g_IPanel->GetName(panel);
        if (strlen(name) > 5)
        {
            if (name[0] == 'F' && name[5] == 'O')
            {
                panel_focus = panel;
            }
        }
    }
    if (!panel_scope)
    {
        name = g_IPanel->GetName(panel);
        if (!strcmp(name, "HudScope"))
        {
            panel_scope = panel;
        }
    }
    if (!g_IEngine->IsInGame())
    {
        g_Settings.bInvalid = true;
    }

    if (panel != panel_focus)
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
    draw::UpdateWTS();
}
}
