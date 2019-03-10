#pragma once
#include <functional>
#include <stdio.h>
#include <string.h>
#include "core/logging.hpp"
#include <sys/mman.h>

class BytePatch
{
    void *addr{ 0 };
    size_t size;
    std::vector<unsigned char> patch_bytes;
    std::vector<unsigned char> original;

public:
    ~BytePatch()
    {
        Shutdown();
    }
    BytePatch(std::function<uintptr_t(const char *)> SigScanFunc, const char *pattern, size_t offset, std::vector<unsigned char> patch) : patch_bytes{ patch }
    {
        addr = (void *) SigScanFunc(pattern);
        if (!addr)
        {
            logging::Info("Signature not found");
            throw std::runtime_error("Signature not found");
        }
        addr = static_cast<void *>(static_cast<char *>(addr) + offset);
        size = patch.size();
        original.resize(size);
        memcpy(&original[0], addr, size);
    }
    BytePatch(uintptr_t addr, std::vector<unsigned char> patch) : addr{ reinterpret_cast<void *>(addr) }, patch_bytes{ patch }
    {
        size = patch.size();
        original.resize(size);
        memcpy(&original[0], reinterpret_cast<void *>(addr), size);
    }
    BytePatch(void *addr, std::vector<unsigned char> patch) : addr{ addr }, patch_bytes{ patch }
    {
        size = patch.size();
        original.resize(size);
        memcpy(&original[0], addr, size);
    }

    void Patch()
    {
        void *page = (void *) ((uint64_t) addr & ~0xFFF);
        logging::Info("mprotect: %d", mprotect(page, 0xFFF, PROT_READ | PROT_WRITE | PROT_EXEC));
        memcpy(addr, &patch_bytes[0], size);
        logging::Info("mprotect reverse: %d", mprotect(page, 0xFFF, PROT_EXEC));
    }
    void Shutdown()
    {
        void *page = (void *) ((uint64_t) addr & ~0xFFF);
        logging::Info("mprotect: %d", mprotect(page, 0xFFF, PROT_READ | PROT_WRITE | PROT_EXEC));
        memcpy(addr, &original[0], size);
        logging::Info("mprotect reverse: %d", mprotect(page, 0xFFF, PROT_EXEC));
    }
};
