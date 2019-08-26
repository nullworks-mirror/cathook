/*
 * entry.cpp
 *
 *  Created on: Oct 3, 2016
 *      Author: nullifiedcat
 */

#include "common.hpp"
#include <thread>
#include <atomic>

#include "hack.hpp"

static std::atomic isStopping = false;
static std::thread thread_main;

void *MainThread()
{
    std::this_thread::sleep_for(std::chrono_literals::operator""s(50));
    hack::Initialize();
    logging::Info("Init done...");
    while (!isStopping)
    {
        std::this_thread::sleep_for(std::chrono_literals::operator""ms(500));
    }
    hack::Shutdown();
    logging::Shutdown();
    return nullptr;
}

void __attribute__((constructor)) attach()
{
    thread_main = std::thread(MainThread);
}

void detach()
{
    logging::Info("Detaching");
    isStopping = true;
    thread_main.join();
}

void __attribute__((destructor)) deconstruct()
{
    detach();
}

CatCommand cat_detach("detach", "Detach cathook from TF2", []() {
    hack::game_shutdown = false;
    detach();
});
