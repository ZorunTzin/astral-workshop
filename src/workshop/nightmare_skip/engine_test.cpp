#include "engine.h"
#include "quests.h"
#include "reward_item.h"
#include "../combat/src/curses_native.h"
#include <windows.h>
#include <fstream>
#include <cstring>
#include <array>
#include <cassert>
#include <iostream>
using namespace NightmareSkip;
int main() {
    assert(rewardOwnedBy(0x12345678,0x12345678,0x98765432));
    assert(rewardOwnedBy(0x12345678,0x98765432,0x12345678));
    assert(!rewardOwnedBy(0x12345678,0x98765432,0x11111111));
    assert(!rewardOwnedBy(0,0,0));

    // Validate the actual runtime request against the public loader contract.
    // Native identified flag 0x10 must never leak into ItemService stateFlags.
    const auto reward=rewardItemSpec(0x12345678u);
    assert((reward.stateFlags & ~D2RL::Items::AllItemStates)==0);
    assert(reward.stateFlags & D2RL::Items::ItemStateIdentified);
    assert(reward.quantity==D2RL::Items::DefaultValue);
    assert(reward.durability==D2RL::Items::DefaultValue);
    assert(reward.structSize>=D2RL::Items::ItemCreateSpecRequiredSize);
    assert(reward.code==D2RL::Items::MakeItemCode("Wnl"));
    assert(reward.seedMode==D2RL::Items::SeedMode::Deterministic);
    assert(reward.generationSeed==0x12345678u && reward.itemSeed==0x12345678u);
    assert(reward.destination.container==D2RL::Items::ItemContainer::Inventory);
    assert(reward.destination.placement==D2RL::Items::Placement::Automatic);
    std::cout << "Nightmare Legacy: valid loader flags, bound seed and inventory destination passed\n";

    // Execute the captured native accessor against an isolated unit. A timer
    // decoy catches accidentally binding UnitTimerRva instead of GameRva.
    std::ifstream snapshot("archive/local/research/native-aura-snapshot.bin",std::ios::binary);
    assert(snapshot.good());
    unsigned char code[0x3b]{};
    snapshot.seekg(CurseNative::GameRva);snapshot.read(reinterpret_cast<char*>(code),sizeof(code));
    assert(snapshot.gcount()==sizeof(code));
    assert(!std::memcmp(code,CurseNative::GameBytes,sizeof(CurseNative::GameBytes)));
    auto executable=VirtualAlloc(nullptr,sizeof(code),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);assert(executable);
    std::memcpy(executable,code,sizeof(code));DWORD oldProtect{};
    assert(VirtualProtect(executable,sizeof(code),PAGE_EXECUTE_READ,&oldProtect));
    FlushInstructionCache(GetCurrentProcess(),executable,sizeof(code));
    alignas(16) unsigned char unit[0x200]{},game[0x200]{},timer[0x200]{};
    void* gp=game;void* tp=timer;
    std::memcpy(unit+0xd8,&gp,sizeof(gp));std::memcpy(unit+0x148,&tp,sizeof(tp));
    using NativeGame=void*(__fastcall*)(void*);
    assert(reinterpret_cast<NativeGame>(executable)(unit)==game);
    tp=nullptr;std::memcpy(unit+0x148,&tp,sizeof(tp));
    assert(reinterpret_cast<NativeGame>(executable)(unit)==game);
    VirtualFree(executable,0,MEM_RELEASE);
    std::cout << "Native game accessor: correct context with present and absent event timer passed\n";

    // Execute the actual game-scoped native lookup and its hash helper, keeping
    // their relative positions intact. No global/client unit table is involved.
    constexpr unsigned findRva=0x48fe80,helperEnd=0x492620;
    std::array<unsigned char,helperEnd-findRva> findCode{};
    snapshot.seekg(findRva);snapshot.read(reinterpret_cast<char*>(findCode.data()),findCode.size());
    assert(snapshot.gcount()==findCode.size());
    executable=VirtualAlloc(nullptr,findCode.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);assert(executable);
    std::memcpy(executable,findCode.data(),findCode.size());
    assert(VirtualProtect(executable,findCode.size(),PAGE_EXECUTE_READ,&oldProtect));
    FlushInstructionCache(GetCurrentProcess(),executable,findCode.size());
    alignas(16) unsigned char activeGame[0x4000]{},otherGame[0x4000]{},collision[0x200]{};
    unsigned playerId=129,collisionId=1;
    std::memcpy(unit+8,&playerId,4);std::memcpy(collision+8,&collisionId,4);
    void* player=unit;void* head=collision;
    std::memcpy(collision+0x158,&player,8);std::memcpy(activeGame+0x2238,&head,8);
    using NativeFind=void*(__fastcall*)(void*,int,unsigned);
    auto find=reinterpret_cast<NativeFind>(executable);
    assert(find(activeGame,0,129)==unit);assert(find(activeGame,0,1)==collision);
    assert(find(activeGame,0,257)==nullptr);assert(find(otherGame,0,129)==nullptr);
    VirtualFree(executable,0,MEM_RELEASE);
    std::cout << "Native server lookup: current game, hash collision, missing player and other game passed\n";
    // Runtime readiness: an early quest notification must not discard eligibility.
    bool waiting=eligibleBaseline(0,4,0,true);assert(waiting);
    assert(!completionReady(waiting,0,4,0x2001,true));
    assert(completionReady(waiting,0,5,0x2001,true));
    assert(!completionReady(false,0,5,0x2001,true));
    assert(!eligibleBaseline(0,5,0x8001,true)); // old Normal completion
    assert(!eligibleBaseline(1,5,0,true)); // already in Nightmare
    assert(!eligibleBaseline(0,4,0,false)); // prior Nightmare quest progress
    std::array<unsigned short,48> flags{};
    assert(untouchedNightmare(flags));
    flags[34]=0x1234;flags[47]=0x5678;
    completeNightmare(flags);
    assert(flags[34]==0x1234&&flags[47]==0x5678);
    assert(rewardPending(flags));assert(!untouchedNightmare(flags));
    // Native NPCs and quest consumables cannot award a second bonus.
    for(unsigned q:QuestRows){assert(flags[q]&1);assert(!(flags[q]&2));}
    assert(!(flags[9]&(1<<5)));assert(!(flags[20]&(1<<5)));
    assert(!(flags[37]&(1<<8)));
    flags[37]|=1<<7;assert(!rewardPending(flags));
    for(auto& word:flags)word&=~((1<<13)|(1<<14)); // native load reset
    assert(!rewardPending(flags));
    D2RL::Items::ItemInfo requested{};
    requested.code=D2RL::Items::MakeItemCode("Wnsk");
    requested.runtimeId=123;requested.itemSeed=456;
    requested.container=D2RL::Items::ItemContainer::Inventory;requested.handle=1;
    auto serverItem=requested;serverItem.handle=99;
    assert(sameRewardItem(requested,serverItem)); // different UI/server handles
    serverItem.runtimeId++;assert(!sameRewardItem(requested,serverItem));
    serverItem=requested;serverItem.itemSeed++;assert(sameRewardItem(requested,serverItem)); // client/server RNG state may differ
    serverItem=requested;serverItem.container=D2RL::Items::ItemContainer::Cube;
    assert(!sameRewardItem(requested,serverItem));
    Completion first{Normal,BaalQuest,false,true,false};
    for (unsigned difficulty=0;difficulty<3;++difficulty)
      for (unsigned quest=0;quest<48;++quest)
       for (int before=0;before<2;++before)
        for (int after=0;after<2;++after)
         for (int higher=0;higher<2;++higher) {
          Record r;
          bool expected=difficulty==0 && quest==40 && !before && after && !higher;
          assert(offer(r,{difficulty,quest,bool(before),bool(after),bool(higher)})==expected);
         }
    Record normal; assert(offer(normal,first));
    assert(choose(normal,Choice::Nightmare,false,0));
    assert(!choose(normal,Choice::Hell,true,123));
    assert(!consume(normal,123)); assert(!offer(normal,first));
    Record skip; assert(offer(skip,first)); assert(!offer(skip,first));
    assert(!choose(skip,Choice::Hell,false,123));
    assert(skip.state==State::Offered && skip.token==0);
    assert(!choose(skip,Choice::Hell,true,0));
    assert(choose(skip,Choice::Hell,true,123));
    assert(!choose(skip,Choice::Hell,true,456));
    assert(!consume(skip,456)); assert(!consume(skip,0));
    Record reloaded=skip;
    assert(consume(reloaded,123)); assert(!consume(reloaded,123));
    assert(!offer(reloaded,first));
    Record other; assert(!consume(other,123));
    Record transaction=skip; assert(consume(transaction,123));
    // Aborted native transaction must not alter its persisted source record.
    assert(skip.state==State::RewardPending);
    assert(NightmareReward.skillPoints==4 && NightmareReward.allResistances==10 &&
           NightmareReward.attributePoints==5 && NightmareReward.maximumLife==20);
    std::cout << "Nightmare skip policy: eligibility, decline, inventory retry, ownership, replay and transaction abort passed\n";
}
