#pragma once
#include "bindings.h"
#include "native_base.h"
// Phase-one native spell bridge; new modifier rolls are added separately.
namespace MonsterModifierSpells {
using LearnFn=void(__fastcall*)(void*,int,int,int);
inline LearnFn Learn=nullptr;
inline unsigned Depth=0,Reports=0;
inline void* CastingMonster=nullptr;
inline void* CastingTarget=nullptr;
inline bool PrivateMissile(int id){return id>=GiftedFireMissile&&id<=EnchantedColdMissile;}
inline bool BeforeCreate(void*,void* raw) noexcept {
 auto* p=static_cast<Pulse::Params*>(raw);
 if(!p||!Depth||!CastingMonster||!CastingTarget||!PrivateMissile(p->missile))return true;
 // Item-proc cast helpers may anchor at the victim. Use an explicit world-space
 // start and destination for the initial volley. Later chain bounces retain
 // their impact origin because they run outside this synchronous cast scope.
 p->owner=CastingMonster;p->origin=CastingMonster;
 p->x=Assassin::Coordinate(CastingMonster,false);p->y=Assassin::Coordinate(CastingMonster,true);
 p->targetX=Assassin::Coordinate(CastingTarget,false);p->targetY=Assassin::Coordinate(CastingTarget,true);
 p->target=nullptr;p->flags=(p->flags|0x21U)&~0x402U;
 return true;
}
inline std::vector<void*> CastMissiles;
inline void Created(void*,void* raw,void* missile){if(!raw||!missile||CastMissiles.size()>=64)return;auto* p=static_cast<Pulse::Params*>(raw);if(Curses::Monster(p->owner)&&p->missile>=GiftedFireMissile&&p->missile<=EnchantedColdMissile){if(Depth)CastMissiles.push_back(missile);else if(Amazon::PublishMissiles){void* list[]={missile};Amazon::PublishMissiles(p->owner,list,1,nullptr,nullptr,0);}}}
constexpr int Skills[]={GiftedFire,GiftedLightning,GiftedCold,EnchantedChain,EnchantedCold};
inline int Cast(void* game,void* monster,void* target,unsigned spell){
 if(Depth||spell>=5||!Learn||!Curses::Cast||!Assassin::GetTarget||!Assassin::SetTarget)return 0;
 if(!GuardianCombat::Hostile(game,monster)||!AuraFeatures::Alive(target)||!Barbarian::SameAuraArea(monster,target))return 0;
 const int level=std::clamp(Ramp::GetUnit(monster,12,0),1,1000),skill=Skills[spell];
 // Same learned-skill setter used by native Teleportation affix initialization.
 // Bind a real learned node, then restore the monster's active AI skill/target.
 Learn(monster,skill,level,1);
 GuardianCombat::SkillScope selected(monster,skill);if(!selected.valid)return 0;
 void* previous=Assassin::GetTarget(game,monster);Assassin::SetTarget(monster,target);
 CastMissiles.clear();CastingMonster=monster;CastingTarget=target;++Depth;const int result=Curses::Cast(monster,skill,level,target,0);--Depth;CastingMonster=nullptr;CastingTarget=nullptr;
 if(Amazon::PublishMissiles&&!CastMissiles.empty())Amazon::PublishMissiles(monster,CastMissiles.data(),static_cast<unsigned char>(CastMissiles.size()),nullptr,nullptr,0);
 CastMissiles.clear();
 Assassin::SetTarget(monster,previous);
 if(Ramp::Log&&Reports++<32){char text[180]{};std::snprintf(text,sizeof(text),"Monster affix spell: unit=%u skill=%d level=%d result=%d",Barbarian::Id(monster),skill,level,result);Ramp::Log(text);}
 return result;
}
inline void* Target(void* game,void* monster){
 void* best=nullptr;double distance=30.0*30.0;
 for(int type:{0,1})AuraFeatures::Units(game,type,[&](void* candidate){
  if(candidate==monster||!AuraFeatures::Alive(candidate)||!Barbarian::SameAuraArea(monster,candidate))return;
  // Friendly/neutral NPCs are not a player's combat unit. Hostile monsters
  // target players and their actual pets/mercenaries, never one another.
  if(type==1&&!Curses::Player(Curses::SkeletonOwner(game,candidate)))return;
  const double dx=Assassin::Coordinate(candidate,false)-Assassin::Coordinate(monster,false);
  const double dy=Assassin::Coordinate(candidate,true)-Assassin::Coordinate(monster,true);
  const double d=dx*dx+dy*dy;if(d<distance){distance=d;best=candidate;}
 });return best;
}
struct Schedule {unsigned id=0xffffffffu,seen=0,gifted=0,chain=0,cold=0,seed=1;};
inline std::array<Schedule,512> Scheduled{};
inline void* Session=nullptr;inline unsigned LastFrame=0,NextScan=0;
inline void Frame(void* game){
 const unsigned now=Ramp::Frame(game);
 if(game!=Session||now<LastFrame){Session=game;Scheduled={};NextScan=0;}
 LastFrame=now;if(Depth||now<NextScan)return;NextScan=now+5;
 AuraFeatures::Units(game,1,[&](void* monster){
  const unsigned rank=MonsterModifiersNative::Rank(monster);
  if(!(rank&8)||(rank&4)||!GuardianCombat::Hostile(game,monster))return;
  const bool gifted=MonsterModifiersNative::Has(monster,8),chain=MonsterModifiersNative::Has(monster,17),cold=MonsterModifiersNative::Has(monster,18);
  if(!gifted&&!chain&&!cold)return;
  const unsigned id=Barbarian::Id(monster);Schedule* slot=nullptr;
  for(auto& x:Scheduled)if(x.id==id){slot=&x;break;}
  if(!slot)for(auto& x:Scheduled)if(x.id==0xffffffffu||now>x.seen+250){x={};x.id=id;x.seed=id^0x9138ba72u;x.gifted=x.chain=x.cold=now+25;slot=&x;break;}
  if(!slot)return;slot->seen=now;
  auto attempt=[&](bool active,unsigned& deadline,unsigned which){
   if(!active||now<deadline)return;deadline=now+25;
   if(void* target=Target(game,monster))Cast(game,monster,target,which);
  };
  unsigned which=slot->seed%3;
  if(gifted&&now>=slot->gifted){slot->seed=slot->seed*1664525u+1013904223u;which=(slot->seed>>8)%3;}
  attempt(gifted,slot->gifted,which);attempt(chain,slot->chain,3);attempt(cold,slot->cold,4);
 });
}
inline bool Bind(const D2RL::PluginContext* context){
 const unsigned char bytes[]={0x48,0x89,0x5c,0x24,0x18,0x55,0x56,0x41,0x54,0x41,0x56,0x41,0x57};
 if(!context->api->checkExpectedBytes(context,0x433910,bytes,sizeof(bytes)))return false;
 Learn=reinterpret_cast<LearnFn>(context->exeBase+0x433910);return true;
}
}
