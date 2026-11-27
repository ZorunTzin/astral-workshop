#pragma once
namespace LeagueTraits {
#include "traits_bindings.h"
enum Trait {None,Giant,Plague,Warden,Brood,Child,Hunter};
inline void* Game=nullptr;inline unsigned Last=0,Seed=731;
inline unsigned Roll(){Seed^=Seed<<13;Seed^=Seed>>17;Seed^=Seed<<5;return Seed;}
inline const TraitClass* Info(int cls){for(auto& c:TraitClasses)if(c.cls==cls)return &c;return nullptr;}
inline int Kind(int cls){for(auto& c:TraitClasses)if(c.cls==cls||c.base==cls)return c.kind;return -1;}
inline int Variant(int base,int trait){for(auto& c:TraitClasses)if(c.base==base&&c.trait==trait)return c.cls;return base;}
inline int Choose(int cls){if(Info(cls))return cls;int kind=Kind(cls),n=Roll()%3,trait=kind==0?(n==0?Giant:n==1?Plague:None):kind==1?(n==0?Warden:n==1?Brood:None):kind==2&&n==0?Hunter:None;return trait?Variant(cls,trait):cls;}
struct Actor {unsigned id=0,owner=0,event=~0u;int cls=0,base=0,trait=0,area=0,wave=0,x=0,y=0,hp=0;std::array<int,16> damage{};bool dead=false,handled=false;int children=0;unsigned huntUntil=0,target=0;bool fast=false;};
inline std::vector<Actor> Actors;
inline void (*TrackChild)(void*,unsigned)=nullptr;
inline void Reset(void* game){unsigned now=Ramp::Frame(game);if(Game!=game||now<Last){Actors.clear();Game=game;Seed=unsigned(GetTickCount())|1;}Last=now;}
inline Actor* Find(void* u){if(!u)return nullptr;unsigned id=Barbarian::Id(u);int cls=int(static_cast<unsigned*>(u)[1]);for(auto& a:Actors)if(a.id==id&&a.cls==cls)return &a;return nullptr;}
inline void Attach(void* game,void* u,void* owner,unsigned event=~0u){Reset(game);auto c=Info(int(static_cast<unsigned*>(u)[1]));if(!c||Find(u))return;
 Actor a;a.id=Barbarian::Id(u);a.owner=Barbarian::Id(owner);a.event=event;a.cls=c->cls;a.base=c->base;a.trait=c->trait;a.area=Arena::Level(u);a.wave=Arena::ArenaLevel(u)?Arena::Current.wave:0;a.x=Assassin::Coordinate(u,false);a.y=Assassin::Coordinate(u,true);
 if(a.trait==Giant){int hp=int(std::min(0x3fffffffLL,1LL*Ramp::GetUnit(u,7,0)*250/100));Ramp::SetUnit(u,7,hp,0);Ramp::SetUnit(u,6,hp,0);Ramp::SetUnit(u,67,Ramp::GetUnit(u,67,0)-35,0);}
 if(a.trait==Plague){int poison=int(std::max(256LL,1LL*Ramp::GetUnit(u,22,0)*3/100));Ramp::SetUnit(u,57,poison,0);Ramp::SetUnit(u,58,poison,0);Ramp::SetUnit(u,59,100,0);}
 a.hp=Ramp::GetUnit(u,7,0);for(unsigned i=0;i<std::size(Amazon::DamageStats);++i)a.damage[i]=Ramp::GetUnit(u,Amazon::DamageStats[i],0);Actors.push_back(a);
}
inline void Death(unsigned id,int cls){for(auto& a:Actors)if(a.id==id&&a.cls==cls)a.dead=true;}
inline bool Pending(const Actor& a){if(a.trait!=Brood)return false;auto u=Curses::Find(Game,1,a.id);return a.children>0||(!a.handled&&(a.dead||(u&&!MonsterRuntime::Living(u))));}
inline void Cancel(unsigned event){for(auto& a:Actors)if(a.event==event){a.handled=true;a.children=0;}}
inline bool Busy(unsigned event){for(auto& a:Actors)if(a.event==event&&Pending(a))return true;return false;}
inline void Damage(void* game,void* attacker,void* defender,void* packet){if(game!=Game)return;
 if(auto a=Find(attacker)){
  if(a->trait==Giant)AuraFeatures::ScaleResolvedDamage(packet,150,100);
  if(a->trait==Child)AuraFeatures::ScaleResolvedDamage(packet,50,100);
  if(a->trait==Hunter&&Curses::Player(defender)&&(Barbarian::ReadDamage(packet,4)&1)&&Barbarian::ReadDamage(packet,0x134)>0){a->huntUntil=Ramp::Frame(game)+125;a->target=Barbarian::Id(defender);}
 }
 if(auto a=Find(defender);a&&a->trait==Warden){int dx=Assassin::Coordinate(defender,false)-a->x,dy=Assassin::Coordinate(defender,true)-a->y;if(dx*dx+dy*dy<=144)AuraFeatures::ScaleResolvedDamage(packet,50,100);}
}
inline void Frame(void* game){Reset(game);unsigned now=Ramp::Frame(game);const auto count=Actors.size();
 for(unsigned i=0;i<count;++i){auto& a=Actors[i];auto u=Curses::Find(game,1,a.id);if(u&&int(static_cast<unsigned*>(u)[1])!=a.cls)u=nullptr;
  bool active=!a.wave||(Arena::Game==game&&Arena::Current.wave==a.wave&&Arena::Current.stage==a.area-Arena::FirstLevel+1&&(Arena::Current.phase==Arena::Phase::Fighting||Arena::Current.phase==Arena::Phase::Countdown));
  if(!active){a.handled=true;a.children=0;continue;}
  if(a.trait==Hunter&&u&&MonsterRuntime::Living(u)){bool fast=now<a.huntUntil;if(fast!=a.fast){Ramp::SetUnit(u,67,Ramp::GetUnit(u,67,0)+(fast?60:-60),0);a.fast=fast;}if(fast)if(auto target=Curses::Find(game,0,a.target))if(Arena::Level(target)==a.area)Assassin::SetTarget(u,target);}
  if(a.trait!=Brood)continue;
  if(!a.handled&&(a.dead||(u&&!MonsterRuntime::Living(u)))){a.handled=true;a.children=2;if(u){a.x=Assassin::Coordinate(u,false);a.y=Assassin::Coordinate(u,true);}}
  if(!a.children)continue;auto owner=Curses::Find(game,0,a.owner);if(!owner||Arena::Level(owner)!=a.area)continue;
  int x=a.x+(a.children==2?-2:2),y=a.y;int cls=Variant(a.base,Child);
  if(a.wave&&!Arena::SafePosition(x,y))continue;
  auto child=MonsterRuntime::Spawn(game,SoulHarvest::Room(owner),x,y,cls,1,-1,0);if(!child)continue;
  if(Arena::Level(child)!=a.area||(a.wave&&!Arena::SafePosition(Assassin::Coordinate(child,false),Assassin::Coordinate(child,true)))){if(Barbarian::RoomDelete)Barbarian::RoomDelete(SoulHarvest::Room(child),1,Barbarian::Id(child));Barbarian::DeleteUnit(game,child);continue;}
  if(a.wave){Arena::WaveProgression(child);Arena::MonsterBonuses(child);}
  int hp=std::max(256,int(1LL*a.hp*35/100));Ramp::SetUnit(child,7,hp,0);Ramp::SetUnit(child,6,hp,0);Ramp::SetUnit(child,13,0,0);
  for(unsigned j=0;j<std::size(Amazon::DamageStats);++j)Ramp::SetUnit(child,Amazon::DamageStats[j],a.damage[j],0);
  Ramp::SetUnit(child,67,Ramp::GetUnit(child,67,0)+60,0);Ramp::SetUnit(child,68,Ramp::GetUnit(child,68,0)+30,0);Assassin::SetTarget(child,owner);
  auto event=a.event;--a.children;
  if(a.wave){Arena::Enemies.push_back(Barbarian::Id(child));Arena::SpawnRecords.push_back({Barbarian::Id(child),cls,0,0});}
  else if(TrackChild)TrackChild(child,event);
  Attach(game,child,owner,event); // May reallocate Actors. Do not touch a afterwards.
 }
 Actors.erase(std::remove_if(Actors.begin(),Actors.end(),[](const Actor& a){return a.dead&&(a.trait!=Brood||(a.handled&&!a.children));}),Actors.end());
}
}
