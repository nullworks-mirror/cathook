/*
  Created on 01.07.18.
*/

#pragma once

#include "Settings.hpp"
#include "Manager.hpp"

namespace settings
{

template<>
class Variable<bool>: public VariableBase<bool>
{
public:
    ~Variable() override = default;

    Variable() = default;

    explicit Variable(bool v)
    {
        setInternal(v);
    }

    VariableType getType() override
    {
        return VariableType::BOOL;
    }

    void fromString(const std::string& string) override
    {
        if (string == "0" || string == "false")
            setInternal(false);
        else if (string == "1" || string == "true")
            setInternal(true);
    }

    Variable<bool>& operator=(const std::string& string)
    {
        fromString(string);
    }

    const std::string &toString() override
    {
        return string;
    }

    explicit operator bool() const
    {
        return value;
    }
    Variable<bool>& operator=(bool next)
    {
        setInternal(next);
    }

    const bool &operator*() override
    {
        return value;
    }

    void flip()
    {
        setInternal(!value);
    }

protected:
    void setInternal(bool next)
    {
        if (next == value)
            return;
        fireCallbacks(next);
        value = next;
        string = value ? "true" : "false";
    }

    bool value{ false };
    std::string string{ "false" };
};

}