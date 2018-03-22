/*
 * CTextLabel.cpp
 *
 *  Created on: Jan 26, 2017
 *      Author: nullifiedcat
 */

#include "menu/CTextLabel.h"

#include "common.hpp"
#include "sdk.hpp"

std::string WordWrap(std::string &in, int max, unsigned long font)
{
    std::stringstream result, line, wordstream, next;
    std::string word;
    char ch;
    for (int i = 0; i < in.size(); i++)
    {
        ch = in.at(i);
        if (ch == ' ' || ch == '\n')
        {
            word = wordstream.str();
            // logging::Info("got word: '%s'", word.c_str());
            wordstream.str("");
            auto size = draw::GetStringLength(font, line.str() + word);
            if (size.first >= max)
            {
                // logging::Info("wrapping: '%s'", line.str().c_str());
                result << line.str() << '\n';
                line.str("");
            }
            line << word << ch;
        }
        else
        {
            wordstream << ch;
        }
    }
    word = wordstream.str();
    wordstream.str("");
    auto size = draw::GetStringLength(fonts::MENU, line.str() + word);
    if (size.first >= max)
    {
        result << line.str() << '\n';
        line.str(word);
    }
    else
    {
        line << word;
    }
    result << line.str();
    return result.str();
}

CTextLabel::CTextLabel(std::string name, IWidget *parent, std::string text,
                       bool centered)
    : CBaseWidget(name, parent)
{
    if (centered)
    {
        SetAutoSize(false);
        SetCentered(true);
    }
    else
    {
        SetAutoSize(true);
        SetCentered(false);
    }
    SetText(text);
}

void CTextLabel::SetAutoSize(bool autosize)
{
    Props()->SetBool("autosize", autosize);
}

void CTextLabel::SetCentered(bool centered)
{
    Props()->SetBool("centered", centered);
}

void CTextLabel::SetPadding(int x, int y)
{
    Props()->SetInt("padding_x", x);
    Props()->SetInt("padding_y", y);
}

void CTextLabel::SetText(std::string text)
{
    unsigned long font_handle = Props()->GetInt("font", fonts::MENU);
    Props()->SetString("text", text.c_str());
    auto padding = std::make_pair(Props()->GetInt("padding_x"),
                                  Props()->GetInt("padding_y"));
    auto size = draw::GetStringLength(font_handle, text);
    if (Props()->GetBool("autosize"))
    {
        SetSize(size.first + padding.first * 2,
                size.second + padding.second * 2);
    }
    else
    {
        auto ms = GetMaxSize();
        SetSize(-1, size.second + padding.second * 2);
        if (ms.first > 0)
        {
            std::string txt = WordWrap(text, ms.first - 2 * padding.first,
                                       menu::ncc::font_item);
            auto size2 = draw::GetStringLength(font_handle, txt);
            SetSize(size2.first + padding.first * 2,
                    size2.second + padding.second * 2);
            Props()->SetString("text", txt.c_str());
        }
    }
}

std::string CTextLabel::GetText()
{
    return std::string(Props()->GetString("text", ""));
}

void CTextLabel::Draw(int x, int y)
{
    if (Props()->GetBool("centered"))
    {
        auto size  = GetSize();
        auto ssize = draw::GetStringLength(fonts::MENU, GetText());
        draw::String(fonts::MENU, x + (size.first - ssize.first) / 2,
                     y + (size.second - ssize.second) / 2, colorsint::white, 1,
                     GetText());
    }
    else
        draw::String(fonts::MENU, x, y, colorsint::white, 1, GetText());
}
