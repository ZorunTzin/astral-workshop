#define main PreviousMonsterTests
#include "../combat/aura_test.cpp"
#include "spells.h"
#undef main
#include "runtime.h"
static unsigned dropCalls=0, volleyCalls=0;
static void* monsterPointer=nullptr;
static void* expectedWeights=nullptr;static unsigned rollCalls=0,initCalls=0;
static unsigned consumeCalls=0;static bool consumeSuccess=false;
int main(){
 PreviousMonsterTests();
 using namespace MonsterRuntime;
 ::Frame(100);Reset(game);Effects.clear();Children.clear();Fields.clear();
 ::Unit boss;boss.type=1;boss.id=234;boss.stats[6]=1000;boss.stats[7]=1000;
 alignas(8) unsigned char md[0x50]{};void* raw=md;std::memcpy(reinterpret_cast<unsigned char*>(&boss)+0x10,&raw,8);unsigned short rank=8;std::memcpy(md+0x1a,&rank,2);
 MonsterModifiersNative::AdditionalHas=ExtraHas;
 auto* a=Effects.add(Key(&boss),Bit(Raid),100);assert(a&&a->mask==All);
 assert(MonsterModifiersNative::Has(&boss,28)&&!MonsterModifiersNative::RawHas(&boss,28));
 OriginalDrop=[](void*,void*,void*,unsigned short,std::uintptr_t,std::uintptr_t,std::uintptr_t,std::uintptr_t,std::uintptr_t,std::uintptr_t)->std::uintptr_t{++dropCalls;return 77;};
 assert(Drop(game,&boss,&player,1,0,0,0,0,0,0)==77&&dropCalls==5);
 // Output-buffer users receive one original call; additional loot rolls must
 // never overrun a caller-owned item array.
 Drop(game,&boss,&player,1,0,0,0,1,0,0);assert(dropCalls==6);
 a=Effects.find(Key(&boss));a->mask=Bit(Glitter);Drop(game,&boss,&player,1,0,0,0,0,0,0);assert(dropCalls==8);
 a->mask=0;Drop(game,&boss,&player,1,0,0,0,0,0,0);assert(dropCalls==9);
 OriginalMulti=[](void*,void*,int,int){++volleyCalls;};
 Multi(game,nullptr,29,1);assert(volleyCalls==2&&WideVolley==0);
 Depth=1;Multi(game,nullptr,29,1);assert(volleyCalls==3);Depth=0;
 // The native picker still controls ordinary rolls. Each custom roll consumes
 // exactly one native slot; expansion terminates subsequent rolls for Raid.
 OriginalRoll=[](void*,void*,unsigned char* used,void* weights)->int{
 ++rollCalls;assert(weights);if(expectedWeights)assert(weights==expectedWeights);
 else {const auto* header=static_cast<const std::uintptr_t*>(weights);assert(header[0]==0&&header[1]==0);}
 for(int id:NativeIds)if(!used[id])return id;return 0;};
 unsigned custom=0;unsigned char used[256]{};
 for(unsigned seed=1;seed<2000;++seed){Effects.clear();a=Effects.add(Key(&boss),0,100);a->seed=seed;int result=RollNative(game,&boss,used,nullptr);a=Effects.find(Key(&boss));if(result==3){++custom;assert(a->mask);if(a->mask&Bit(Raid)){assert(a->mask==All);assert(RollNative(game,&boss,used,nullptr)==0);}}else assert(result==5);}
 assert(custom>300&&custom<1000);
 // External native maps are forwarded unchanged, including the reentrant path.
 std::array<std::uintptr_t,2> suppliedWeights{7,42};expectedWeights=suppliedWeights.data();Depth=1;
 assert(RollNative(game,&boss,used,expectedWeights)==5);expectedWeights=nullptr;
 assert(RollNative(game,&boss,used,nullptr)==5);Depth=0;
 // Reproduce the crashing path: a Hell superunique with two fixed modifiers
 // receives a top-up without an external map. Initializers run only once.
 Effects.clear();std::memset(md+0x20,0,9);md[0x20]=9;md[0x21]=18;rank=10;std::memcpy(md+0x1a,&rank,2);game[0x104]=2;
 Extra topup{Key(&boss)};Track(game,&boss);Init=[](void*,int,int){++initCalls;};unsigned beforeRolls=rollCalls;
 CompleteSuperunique(&boss,topup);assert(rollCalls>beforeRolls&&initCalls>0);
 assert(ModifierCount(MaskOf(&boss))>=3);unsigned completedRolls=rollCalls;
 CompleteSuperunique(&boss,topup);assert(rollCalls==completedRolls);
 game[0x104]=0;rank=8;std::memcpy(md+0x1a,&rank,2);std::memset(md+0x20,0,9);

 Effects.clear();a=Effects.add(Key(&boss),Bit(Spectral),100);assert(a);
 a->mask=Bit(Fire)|Bit(Cold)|Bit(Glitter);Extra scaling{Key(&boss)};boss.stats[6]=500;boss.stats[7]=1000;
 ApplyLife(&boss,scaling);assert(boss.stats[7]==1750&&boss.stats[6]==875);
 ApplyLife(&boss,scaling);assert(boss.stats[7]==1750&&boss.stats[6]==875);
 boss.mode=0;assert(!Living(&boss));boss.mode=12;assert(!Living(&boss));boss.mode=1;assert(Living(&boss));
 // Arena enforcement includes ownership and level boundaries.
 SoulHarvest::Room=[](void* u)->void*{return reinterpret_cast<void*>(std::uintptr_t(static_cast<::Unit*>(u)->room));};
 Barbarian::AuraArea=nullptr;Assassin::NativePositions=true;
 Curses::Owner=[](void*,void* u)->void*{return u==&mercenary?&player:nullptr;};
 alignas(8) unsigned char bp[0x80]{},pp[0x80]{},cp[0x80]{};
 auto path=[](::Unit& u,unsigned char* p,int x,int y){std::memcpy(reinterpret_cast<unsigned char*>(&u)+0x38,&p,8);*reinterpret_cast<unsigned short*>(p+2)=static_cast<unsigned short>(x);*reinterpret_cast<unsigned short*>(p+6)=static_cast<unsigned short>(y);};
 path(boss,bp,50,50);path(player,pp,62,50);player.room=boss.room=1;
 Effects.find(Key(&boss))->mask=Bit(Duelist);
 assert(!IgnoreDamage(game,&player,&boss,nullptr));path(player,pp,63,50);assert(IgnoreDamage(game,&player,&boss,nullptr));
 assert(IgnoreDamage(game,&mercenary,&boss,nullptr));path(player,pp,50,50);assert(!IgnoreDamage(game,&mercenary,&boss,nullptr));
 player.room=2;assert(IgnoreDamage(game,&player,&boss,nullptr));player.room=1;
 // A full heal requires a consumed, eligible corpse. Private helpers cannot feed it.
 ::Unit corpse;corpse.type=1;corpse.id=999;corpse.mode=12;corpse.room=1;path(corpse,cp,51,50);
 Curses::OriginalCheck=[](void*,int)->int{return 0;};
 SoulHarvest::Refresh=nullptr;
 RedemptionLife::Original=[](void*,void*,void*,int skill,int level,int skip)->int{assert(skill==131&&level==1&&skip==1);++consumeCalls;return consumeSuccess;};
 boss.stats[6]=500;boss.stats[7]=2500;
 assert(!Eat(&boss,&corpse)&&boss.stats[6]==500&&consumeCalls==1);
 consumeSuccess=true;assert(Eat(&boss,&corpse)&&boss.stats[6]==2500&&consumeCalls==2);
 assert(!Eat(&boss,&corpse)&&consumeCalls==2);
 boss.stats[6]=500;corpse.cls=RuneClass;assert(!Eat(&boss,&corpse)&&consumeCalls==2);
 corpse.cls=3;corpse.mode=1;assert(!Eat(&boss,&corpse));
 // Session rollback clears all modifier ownership; no previous-game IDs leak.
 Children.push_back({Identity(1,333),Key(&boss),Action::Totem,200});::Frame(1);Reset(game);assert(Effects.actors.empty()&&Children.empty());
 puts("PASS: native-slot/custom-pool integration, Raid effect discovery, safe 2x/5x loot forwarding, bounded five-shot pair, and session cleanup.");
}
