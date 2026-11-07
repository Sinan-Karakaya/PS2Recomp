#pragma once

#include <atomic>
#include <cstdint>

// Counts the guest's display buffer changes (a new DISPFB through the GS
// display environment), so a presenter can show every frame a game finishes,
// not only one per VBlank: a game paced faster than the VBlank flips more
// often than the vsync tick advances. Lives apart from GSRegisters so that
// struct's layout, which the translated code sees, stays as it is.
namespace ps2_display_flips
{
    inline std::atomic<uint64_t> &counter()
    {
        static std::atomic<uint64_t> flips{0u};
        return flips;
    }

    inline void noteFlip() { counter().fetch_add(1u, std::memory_order_release); }
    inline uint64_t count() { return counter().load(std::memory_order_acquire); }
}
