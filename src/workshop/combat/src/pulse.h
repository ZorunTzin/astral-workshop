#pragma once
#include "ramp.h"
#include <cstddef>
#include <cstdint>

namespace Pulse {
using CreateFn = void*(__fastcall*)(void*, void*);
inline CreateFn OriginalCreate = nullptr;
inline bool (*FilterCreate)(void*, void*) noexcept = nullptr;
inline void (*AfterCreate)(void*, void*, void*) noexcept = nullptr;
inline unsigned Logs = 0;

// Native 3.3.93847 reads a 16-byte graphics descriptor at 0x84, with
// its presence byte at 0x8c. Allocate and zero that tail too: stopping at
// the initialization callback (0x74) lets native code consume stack garbage.
struct Params {
    unsigned flags;
    unsigned padding;
    void* owner;
    void* origin;
    void* target;
    int missile, x, y, targetX, targetY, loops, velocity, skill, level;
    unsigned char reservedTail[0x18];
    int range;
    unsigned char reservedAfterRange[0x10];
    int initializeFunction;
    int initializeArgument;
    unsigned char reservedBeforeGraphics[0x0c];
    unsigned char graphicsDescriptor[0x10];
};

static_assert(offsetof(Params, missile) == 0x20 && offsetof(Params, x) == 0x24
              && offsetof(Params, targetX) == 0x2c && offsetof(Params, skill) == 0x3c
              && offsetof(Params, range) == 0x5c
              && offsetof(Params, initializeFunction) == 0x70
              && offsetof(Params, initializeArgument) == 0x74
              && offsetof(Params, graphicsDescriptor) == 0x84
              && sizeof(Params) == 0x98);

inline void* Create(void* game, void* raw) noexcept {
    void* missile = OriginalCreate(game, raw);
    if (missile && AfterCreate)
        AfterCreate(game, raw, missile);
    return missile;
}

inline void* __fastcall OnCreate(void* game, void* raw) noexcept {
    if (FilterCreate && !FilterCreate(game, raw))
        return nullptr;
    auto* p = static_cast<Params*>(raw);
    if (!p || (p->missile != 1012 && p->missile != 1013))
        return Create(game, raw);
    if (p->missile == 1012) {
        p->x = p->targetX;
        p->y = p->targetY;
    }
    // Use the cast location for manual infection, and the corpse location for
    // spread. Absolute position, no origin/distance-driven flight, no target AI.
    p->flags = (p->flags | 0x21U) & ~0x402U;
    p->target = nullptr;
    p->targetX = p->x + 1;
    p->targetY = p->y;
    void* missile = Create(game, raw);
    if (missile)
        Ramp::OnHit(game, missile, nullptr);
    if (Ramp::Log && Logs++ < 256) {
        char message[160]{};
        std::snprintf(message, sizeof(message),
                      "Virus immediate pulse: missile=%d x=%d y=%d created=%d", p->missile, p->x,
                      p->y, missile ? 1 : 0);
        Ramp::Log(message);
    }
    return missile;
}
} // namespace Pulse
