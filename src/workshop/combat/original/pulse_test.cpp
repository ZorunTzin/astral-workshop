#include "pulse.h"
#include <cassert>
#include <map>
#include <new>
static std::map<void*, int> processed;
static unsigned missile[] = {3, 750, 501, 0};
static unsigned char game[0x180]{};
static int hits = 0, creates = 0;
static Pulse::Params seen{};

static void* __fastcall Create(void*, void* raw) {
    seen = *static_cast<Pulse::Params*>(raw);
    int nativeInit=0,nativeArgument=0;
    std::memcpy(&nativeInit,static_cast<unsigned char*>(raw)+0x70,4);
    std::memcpy(&nativeArgument,static_cast<unsigned char*>(raw)+0x74,4);
    assert(nativeInit==seen.initializeFunction&&nativeArgument==seen.initializeArgument);
    missile[1] = seen.missile;
    ++creates;
    processed[missile] = 0;
    return missile;
}

static int __fastcall Hit(void*, void*, void*) {
    ++hits;
    return 1;
}

static int __fastcall Get(void* unit, int stat, unsigned) {
    assert(stat == 407);
    return processed[unit];
}

static void __fastcall Set(void* unit, int stat, int value, unsigned) {
    assert(stat == 407);
    processed[unit] = value;
}

// Simulate the native missile callback after an immediate cast to catch duplicate damage.
int main() {
    // Reproduce the native reads which were missing from the old test double.
    // Poison adjacent storage so insufficient allocation or incomplete zero
    // initialization cannot silently look like a valid no-callback request.
    static_assert(sizeof(Pulse::Params)==0x98);
    alignas(Pulse::Params) unsigned char storage[sizeof(Pulse::Params)+16];
    std::memset(storage,0xa5,sizeof(storage));
    auto* full=new(storage) Pulse::Params{};
    for(unsigned offset=0x60;offset<0x98;++offset)assert(storage[offset]==0);
    for(unsigned offset=0x98;offset<sizeof(storage);++offset)assert(storage[offset]==0xa5);
    assert(full->initializeFunction==0&&full->initializeArgument==0);
    // Native 0x3bcda0 inspects descriptor+8 before 0x3bd720 copies 16 bytes.
    assert(storage[0x8c]==0);
    for(unsigned offset=0x84;offset<0x94;++offset)assert(storage[offset]==0);
    Pulse::OriginalCreate = &Create;
    Ramp::OriginalHit = &Hit;
    Ramp::GetUnit = &Get;
    Ramp::SetUnit = &Set;
    Pulse::Params p{};
    p.missile = 750;
    p.flags = 0x420;
    p.x = 10;
    p.y = 20;
    p.targetX = 100;
    p.targetY = 200;
    p.target = reinterpret_cast<void*>(1);
    assert(Pulse::OnCreate(game, &p) == missile);
    assert(seen.x == 100 && seen.y == 200 && seen.target == nullptr && seen.flags == 0x21);
    assert(hits == 1);
    assert(Ramp::OnHit(game, missile, nullptr) == 1 && hits == 1);
    p = {};
    p.missile = 751;
    p.flags = 0x21;
    p.x = 300;
    p.y = 400;
    p.targetX = 500;
    p.targetY = 600;
    Pulse::OnCreate(game, &p);
    assert(seen.x == 300 && seen.y == 400 && hits == 2);
    Ramp::OnHit(game, missile, nullptr);
    assert(hits == 2);
    p.missile = 749;
    p.x = 17;
    p.targetX = 44;
    p.flags = 0x420;
    Pulse::OnCreate(game, &p);
    assert(seen.x == 17 && seen.flags == 0x420 && hits == 2 && creates == 3);
    // Existing native callers may legitimately provide callback metadata.
    // The wrapper must preserve it, not erase native initialization fields.
    p.initializeFunction=3;p.initializeArgument=42;
    Pulse::OnCreate(game,&p);
    assert(seen.initializeFunction==3&&seen.initializeArgument==42);
    std::puts(
        "PASS: full 0x98-byte native missile block, zeroed callback tail, preserved native callbacks; first cast applies immediately at cursor, spread at corpse, next missile update cannot double-hit, original Pestfluch projectile unchanged.");
}
