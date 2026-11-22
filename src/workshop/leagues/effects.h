#pragma once
#include "craft_bindings.h"
#include "effect_bindings.h"
namespace LeagueEffects {
inline thread_local unsigned Depth=0;
inline bool Weapon(void* attacker){
 if(auto* missile=Amazon::CurrentHitMissile){int id=static_cast<unsigned*>(missile)[1];return std::find(std::begin(LeagueCraft::WeaponMissiles),std::end(LeagueCraft::WeaponMissiles),id)!=std::end(LeagueCraft::WeaponMissiles);}
 int skill=Barbarian::UsedSkill(attacker);
 return skill==0||skill==1||skill==2||std::find(std::begin(LeagueCraft::WeaponSkills),std::end(LeagueCraft::WeaponSkills),skill)!=std::end(LeagueCraft::WeaponSkills);
}
inline void Before(void* game,void* attacker,void* defender,void* packet){
 if(Depth||!packet||!Curses::Player(attacker)||!GuardianCombat::Hostile(game,defender)||Weapon(attacker))return;
 int bonus=std::max(0,Ramp::GetUnit(attacker,LeagueCraft::SpellStat,0));
 if(bonus)AuraFeatures::ScaleResolvedDamage(packet,100+bonus,100);
}
struct Pending {unsigned owner;int area,x,y,low,high;};
inline std::vector<Pending> Queue;
inline void* Game=nullptr;inline unsigned Last=0;
inline void Reset(void* game){unsigned now=Ramp::Frame(game);if(Game!=game||now<Last){Queue.clear();Game=game;}Last=now;}
inline void Nova(void* game,void* owner,const Pending& hit){
 ++Depth;++Curses::ProcDepth;
 // Visual-only expanding rings; one authoritative damage packet per enemy.
 const int x=hit.x,y=hit.y;
 for(int visual:NovaVisuals)for(int n=0;n<32;++n){double a=6.283185307179586*n/32;
  Druid::Visual(game,owner,visual,x,y,x+int(30*std::cos(a)),y+int(30*std::sin(a)),20);
 }
 AuraFeatures::Units(game,1,[&](void* target){
  if(!GuardianCombat::Hostile(game,target)||!AuraFeatures::Alive(target)||Barbarian::AuraArea(target)!=hit.area||std::int64_t(Amazon::X(target)-x)*(Amazon::X(target)-x)+std::int64_t(Amazon::Y(target)-y)*(Amazon::Y(target)-y)>24*24)return;
  Pulse::Params p{};p.flags=0x8021;p.owner=owner;p.origin=owner;p.target=target;p.missile=NovaHit;p.level=60;p.x=Amazon::X(target)-1;p.y=Amazon::Y(target);p.targetX=p.x+2;p.targetY=p.y;p.range=1;
  if(void* missile=Pulse::OnCreate(game,&p)){
   const int low=hit.low,high=hit.high;
   for(int stat:Amazon::DamageStats)Ramp::SetUnit(missile,stat,0,0);
   for(int stat:{48,50,54}){Ramp::SetUnit(missile,stat,low,0);Ramp::SetUnit(missile,stat+1,high,0);}
   Ramp::SetUnit(missile,56,25,0);
   Curses::NativeMissileHit(game,missile,target,0);
  }
 });
 --Curses::ProcDepth;--Depth;
}
inline void Hit(void* game,void* attacker,void* defender,void* packet){
 if(Depth||Curses::ProcDepth||!packet||!Curses::Player(attacker)||!GuardianCombat::Hostile(game,defender)||!Weapon(attacker))return;
 unsigned short flags=0;std::memcpy(&flags,static_cast<unsigned char*>(packet)+4,2);
 if(!(flags&1)||(flags&0x4020)||Barbarian::ReadDamage(packet,0x134)<=0)return;
 const int chance=std::clamp(Ramp::GetUnit(attacker,LeagueCraft::NovaStat,0),0,100);
 if(!chance||SetSelectors::Draw(100)>=unsigned(chance))return;
 Reset(game);if(Queue.size()>=4096)return;
 // Snapshot native weapon damage now; defer damage until the triggering hit ends.
 Pulse::Params p{};p.flags=0x8021;p.owner=attacker;p.origin=attacker;p.missile=NovaHit;p.level=60;p.x=Amazon::X(attacker);p.y=Amazon::Y(attacker);p.range=1;
 if(void* missile=Pulse::OnCreate(game,&p)){
  int low=std::max(0,Ramp::GetUnit(missile,21,0)),high=std::max(low,Ramp::GetUnit(missile,22,0));
  Queue.push_back({Barbarian::Id(attacker),Barbarian::AuraArea(attacker),p.x,p.y,low,high});
 }
}
inline void Frame(void* game){
 Reset(game);auto pending=std::move(Queue);Queue.clear();
 for(const auto& hit:pending){void* owner=Curses::Find(game,0,hit.owner);if(AuraFeatures::Alive(owner)&&Barbarian::AuraArea(owner)==hit.area)Nova(game,owner,hit);}
}
}
