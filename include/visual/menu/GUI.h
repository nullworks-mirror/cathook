/*
 * GUI.h
 *
 *  Created on: Jan 25, 2017
 *      Author: nullifiedcat
 */

#pragma once

class IWidget;
class CatVar;

// #include "../beforecheaders.h"
#include <string>
// #include "../aftercheaders.h"

#include "visual/menu/ncc/Root.hpp"
#include "visual/menu/ncc/Menu.hpp"

class RootWindow;

extern CatVar gui_color_r;
extern CatVar gui_color_g;
extern CatVar gui_color_b;
extern CatVar gui_color_a;

extern CatVar gui_visible;
extern CatVar gui_draw_bounds;

int NCGUIColor();

class CatGUI
{
public:
    CatGUI();
    ~CatGUI();

    bool Visible();
    void Update();
    void Setup();
    bool ConsumesKey(ButtonCode_t key);

    // TODO NullCore tooltip
    menu::ncc::Root *root_nullcore;

    int last_scroll_value;
    bool m_bConsumeKeys;
    bool m_bKeysInit;
    bool m_bPressedState[ButtonCode_t::BUTTON_CODE_COUNT];
    int m_iPressedFrame[ButtonCode_t::BUTTON_CODE_COUNT];
    int m_iMouseX;
    int m_iMouseY;
    int mouse_dx;
    int mouse_dy;
};

extern CatGUI *g_pGUI;