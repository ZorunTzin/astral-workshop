#define VIRUS_TEST
#include "virus.cpp"
#include <cassert>
#include <vector>

struct Unit {
    unsigned type, txt, id, mode;
};

static Unit units[] = {{0, 0, 10, 0}, {1, 0, 101, 0}, {1, 0, 102, 0}, {1, 0, 103, 0}};
static bool locked = false;
static std::vector<unsigned> infected, exploded;

static void* __fastcall FakeFind(void*, int type, unsigned id) {
    for (auto& u : units)
        if (u.type == unsigned(type) && u.id == id)
            return &u;
    return nullptr;
}

static void __fastcall FakeCast(void* caster, int skill, int level, void* corpse, int flags) {
    assert(caster == &units[0] && skill == 444 && level == 4 && flags == 0);
    infected.push_back(static_cast<Unit*>(corpse)->id);
}

static int __fastcall FakeOriginal(void* game, int event, void* source, void* target, void*,
                                   int skill, int level, std::uintptr_t a8, std::uintptr_t owner) {
    if (skill == 66)
        return 66;
    assert(!locked && skill == 77 && level == 4 && a8 == 0);
    locked = true;
    auto id = static_cast<Unit*>(source)->id;
    assert(!infected.empty() && infected.back() == id);
    exploded.push_back(id);
    if (id == 101) {
        assert(Virus::OnEvent(game, event, &units[2], target, nullptr, 444, level, 0, owner) == 1);
        assert(Virus::OnEvent(game, event, &units[2], target, nullptr, 444, level, 0, owner) == 1);
    }
    if (id == 102)
        assert(Virus::OnEvent(game, event, &units[3], target, nullptr, 444, level, 0, owner) == 1);
    locked = false;
    return 1;
}

// Fake native callbacks let the test verify the order and deduplication of a nested death chain.
int main() {
    Virus::Find = &FakeFind;
    Virus::Cast = &FakeCast;
    Virus::Original = &FakeOriginal;
    const unsigned owner[] = {0, 10};
    assert(Virus::OnEvent(nullptr, 10, &units[1], &units[0], nullptr, 77, 4, 0,
                          reinterpret_cast<std::uintptr_t>(owner))
           == 1);
    assert((exploded == std::vector<unsigned>{101, 102, 103}));
    assert(infected == exploded);
    assert(Virus::Active == nullptr);
    assert(Virus::OnEvent(nullptr, 10, &units[1], &units[0], nullptr, 66, 4, 0,
                          reinterpret_cast<std::uintptr_t>(owner))
           == 66);
    Virus::Chain chain{};
    Virus::Packet p{};
    p.corpse = {1, 101};
    p.skill = 66;
    assert(Virus::Enqueue(chain, p) && chain.size == 1);
    assert(Virus::Enqueue(chain, p) && chain.size == 1);
    p.skill = 77;
    assert(Virus::Enqueue(chain, p) && chain.size == 2);
    p.skill = 86;
    assert(Virus::Enqueue(chain, p) && chain.size == 3);
    // Recover an Infection death which never reached the native event hook.
    alignas(8) unsigned char game[0x3000]{};
    *reinterpret_cast<unsigned*>(game+0x170)=10;
    Ramp::GetUnit=[](void* u,int stat,unsigned)->int{return stat==6&&static_cast<Unit*>(u)->id!=101?256:0;};
    Ramp::OriginalList=[](void*,int)->void*{return &units[1];};
    Ramp::Skill=[](void*)->int{return 77;};
    infected.clear();exploded.clear();
    Virus::Track(game,&units[0],&units[1],4,200);
    Virus::Poll(game);
    assert((exploded==std::vector<unsigned>{101,102,103}));
    Virus::Poll(game);assert(exploded.size()==3);
    Virus::OnEvent(game,10,&units[1],&units[0],nullptr,77,4,0,reinterpret_cast<std::uintptr_t>(owner));
    assert(exploded.size()==3); // A late native callback cannot duplicate it.
    std::puts(
        "PASS: infection precedes explosion; three-generation nested death chain; duplicate death ignored; Pestfluch forwarded; different death curses on the same corpse remain independent.");
}
