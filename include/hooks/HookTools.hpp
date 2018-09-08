#pragma once
#include <vector>
#include <functional>
#include <init.hpp>
#include "core/logging.hpp"

namespace HookTools
{
void CreateMove();
}

struct CreateMove
{
    CreateMove(int priority = 5, std::function<void()> func = []() {});
};
