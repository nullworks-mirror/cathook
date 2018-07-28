/*
  Created on 01.07.18.
*/

#pragma once

#include "Settings.hpp"

namespace settings
{

template<>
class Variable<std::string>: public VariableBase<std::string>
{
public:
    ~Variable() override = default;

    VariableType getType() override
    {
        return VariableType::STRING;
    }

    void fromString(const std::string& string) override
    {
        fireCallbacks(string);
        value = string;
    }

    const std::string &toString() override
    {
        return value;
    }

    Variable<std::string>& operator=(const std::string& string)
    {
        fireCallbacks(std::string(string));
        value = string;
    }

    const std::string &operator*() override
    {
        return value;
    }

    explicit operator bool() const
    {
        return !value.empty();
    }

protected:
    std::string value{};
};

}