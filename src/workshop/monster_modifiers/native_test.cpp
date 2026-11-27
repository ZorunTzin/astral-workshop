#define main OriginalAuraTest
#include "../combat/aura_test.cpp"
#undef main
#include "spells.h"
static unsigned fastCalls=0,resistCalls=0;
static void* selectedTarget=nullptr;
static void* expectedCaster=nullptr;static unsigned published=0;static Unit testMissile;
int main(){
 OriginalAuraTest();
 Unit monster;monster.type=1;monster.stats[6]=1000;
 alignas(8) unsigned char md[0x50]{};void* raw=md;
 std::memcpy(reinterpret_cast<unsigned char*>(&monster)+0x10,&raw,8);
 unsigned short rank=8;std::memcpy(md+0x1a,&rank,2);md[0x20]=28;
 MonsterModifiersNative::OriginalFast=[](void* u,int,int){++fastCalls;Ramp::SetUnit(u,67,Ramp::GetUnit(u,67,0)+60,0);};
 MonsterModifiersNative::OriginalResistance=[](void* u,int mod,int){++resistCalls;if(mod==8)for(int stat:{39,41,43})Ramp::SetUnit(u,stat,Ramp::GetUnit(u,stat,0)+40,0);};
 monster.stats[67]=25;MonsterModifiersNative::Fast(&monster,6,1);assert(monster.stats[67]==225&&monster.stats[68]==200);
 MonsterModifiersNative::Resistance(&monster,8,1);assert(monster.stats[39]==20&&monster.stats[41]==20&&monster.stats[43]==20);
 MonsterModifiersNative::Resistance(&monster,28,1);assert(resistCalls==1);
 // Champions keep their native movement/resistance behavior.
 rank=4;std::memcpy(md+0x1a,&rank,2);monster.stats[67]=0;
 MonsterModifiersNative::Fast(&monster,6,1);assert(monster.stats[67]==60);
 MonsterModifiersNative::Resistance(&monster,8,1);assert(monster.stats[39]==60);
 Curses::Owner=[](void*,void*)->void*{return nullptr;};
 SoulHarvest::Room=[](void*)->void*{return reinterpret_cast<void*>(1);};
 rank=8;std::memcpy(md+0x1a,&rank,2);
 alignas(8) unsigned char packet[0x180]{};
 for(unsigned off:{0x18u,0x20u,0x24u,0x2cu,0x30u,0x34u,0x38u,0x134u}){int n=1000;std::memcpy(packet+off,&n,4);}
 MonsterModifiersNative::Damage(game,&player,&monster,packet);
 for(unsigned off:{0x18u,0x20u,0x24u,0x2cu,0x30u,0x34u,0x38u,0x134u}){int n=0;std::memcpy(&n,packet+off,4);assert(n==250);}
 monster.stats[12]=20;player.stats[6]=1000;selectedTarget=&mercenary;
 Assassin::GetTarget=[](void*,void*)->void*{return selectedTarget;};
 Assassin::SetTarget=[](void*,void* target){selectedTarget=target;};
 MonsterModifierSpells::Learn=[](void*,int skill,int level,int flag){assert(skill==GiftedFire&&level==20&&flag==1);};
 Curses::Cast=[](void*,int skill,int level,void* target,int)->int{assert(skill==GiftedFire&&level==20&&target==&player&&selectedTarget==target);return 1;};
 assert(MonsterModifierSpells::Cast(game,&monster,&player,0)==1&&selectedTarget==&mercenary);
 player.stats[6]=0;assert(MonsterModifierSpells::Cast(game,&monster,&player,0)==0);
 assert(MonsterModifierSpells::Cast(game,&monster,&player,5)==0);
 // Reproduce a proc anchored at the player, then verify server origin AND publisher.
 player.stats[6]=1000;monster.x=30;monster.y=40;player.x=90;player.y=100;expectedCaster=&monster;
 Curses::X=[](void* u){return static_cast<Unit*>(u)->x;};Curses::Y=[](void* u){return static_cast<Unit*>(u)->y;};
 MonsterModifierSpells::Learn=[](void*,int,int,int){};
 Amazon::PublishMissiles=[](void* owner,void** list,unsigned char count,void* origin,void* target,unsigned short){assert(owner==expectedCaster&&count==1&&list[0]==&testMissile&&!origin&&!target);++published;};
 Curses::Cast=[](void* caster,int skill,int,void* target,int)->int{
  assert(caster==expectedCaster&&target==&player);
  Pulse::Params params{};params.owner=params.origin=params.target=&player;params.x=90;params.y=100;params.flags=0x402;
  params.missile=skill==EnchantedCold?EnchantedColdMissile:EnchantedChainMissile;
  assert(MonsterModifierSpells::BeforeCreate(game,&params));
  assert(params.owner==caster&&params.origin==caster&&!params.target);
  assert(params.x==30&&params.y==40&&params.targetX==90&&params.targetY==100&&params.flags==0x21);
  MonsterModifierSpells::Created(game,&params,&testMissile);return 1;
 };
 for(unsigned spell:{3u,4u})assert(MonsterModifierSpells::Cast(game,&monster,&player,spell)==1);
 assert(published==2&&!MonsterModifierSpells::CastingMonster&&!MonsterModifierSpells::CastingTarget&&selectedTarget==&mercenary);
 Pulse::Params bounce{};bounce.missile=EnchantedChainMissile;bounce.owner=&monster;bounce.x=71;bounce.y=83;auto preserved=bounce;
 MonsterModifierSpells::BeforeCreate(game,&bounce);assert(!std::memcmp(&bounce,&preserved,sizeof(bounce)));
 MonsterModifierSpells::Created(game,&bounce,&testMissile);assert(published==3);
 MonsterModifierSpells::Depth=1;MonsterModifierSpells::CastingMonster=&monster;MonsterModifierSpells::CastingTarget=&player;
 Pulse::Params ordinary{};ordinary.missile=1;ordinary.owner=&player;auto original=ordinary;
 MonsterModifierSpells::BeforeCreate(game,&ordinary);assert(!std::memcmp(&ordinary,&original,sizeof(ordinary)));
 MonsterModifierSpells::Depth=0;MonsterModifierSpells::CastingMonster=MonsterModifierSpells::CastingTarget=nullptr;
 // Final missile damage is bounded independently of inherited native values.
 Pulse::Params gifted{};gifted.owner=&monster;gifted.missile=GiftedLightningMissile;
 monster.stats[12]=5;testMissile.stats[50]=testMissile.stats[51]=999999;
 MonsterModifierSpells::BalanceGiftedLightning(&gifted,&testMissile);
 assert(GetUnit(&testMissile,50,0)==50*256&&GetUnit(&testMissile,51,0)==50*256);
 monster.stats[12]=105;MonsterModifierSpells::BalanceGiftedLightning(&gifted,&testMissile);assert(GetUnit(&testMissile,50,0)==250*256);
 puts("PASS: cold/lightning start at monster, fly toward player, publish without victim anchor; later bounces and unrelated missiles unchanged.");
 puts("PASS: +200% replaces native Extra Fast contribution; half elemental resistance bonus; Stone Skin removes its resistance initializer; champions unchanged; invalid spell rejected.");
}
