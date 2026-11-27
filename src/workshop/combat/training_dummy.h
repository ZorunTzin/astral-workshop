#pragma once
namespace TrainingDummy {
#include "training_dummy_bindings.h"
constexpr unsigned Invalid=0xffffffffu;
constexpr int Life=1000000*256;
inline void* Game=nullptr;inline unsigned Last=0,Next=0,HealAt=0,Id=Invalid;
inline bool Placed=false;inline int X=0,Y=0;
struct Visit{unsigned id=Invalid;int area=0;};inline std::array<Visit,16> Visits{};
inline bool Is(void* u){return u&&static_cast<unsigned*>(u)[0]==1&&int(static_cast<unsigned*>(u)[1])==Class;}
inline void Heal(void* u){
 Ramp::SetUnit(u,7,Life,0);Ramp::SetUnit(u,6,Life,0);
 if(SoulHarvest::Refresh)SoulHarvest::Refresh(u);
}
inline void* Spawn(void* game,void* player,int x,int y){
 auto room=League::RoomAt(SoulHarvest::Room(player),x,y);if(!room)return nullptr;
 League::Point at{x,y},found=at;
 if(!League::FreePosition(room,&found,&found,1,0x3e01,0xc01,0)||found.x!=x||found.y!=y)return nullptr;
 auto u=MonsterRuntime::Spawn(game,room,x,y,Class,1,-1,0);
 if(!Is(u))return nullptr;
 if(Arena::Level(u)!=2){Barbarian::RoomDelete(SoulHarvest::Room(u),1,Barbarian::Id(u));Barbarian::DeleteUnit(game,u);return nullptr;}
 // Neutral defences: no resistance, regeneration, blocking or damage reduction.
 for(int stat:{35,36,37,39,41,43,45,74,102})Ramp::SetUnit(u,stat,0,0);
 Heal(u);Curses::Update(u,"Target Dummy",0);return u;
}
inline void Frame(void* game){
 if(!game||!Arena::Level||!MonsterRuntime::Spawn||!League::RoomAt||!League::FreePosition||!Barbarian::RoomDelete||!Barbarian::DeleteUnit)return;
 auto now=Ramp::Frame(game);
 if(Game!=game||now<Last){Game=game;Next=0;HealAt=0;Id=Invalid;Placed=false;Visits={};}Last=now;
 if(now<Next)return;Next=now+5;
 std::vector<unsigned> ids;AuraFeatures::Units(game,0,[&](void* u){if(!AuraBuffs::Dead(u))ids.push_back(Barbarian::Id(u));});
 for(auto id:ids){auto player=Curses::Find(game,0,id);if(!player)continue;int area=Arena::Level(player);
  Visit* visit=nullptr;for(auto& v:Visits)if(v.id==id){visit=&v;break;}
  if(!visit)for(auto& v:Visits)if(v.id==Invalid){visit=&v;v.id=id;break;}
  const bool entering=visit&&visit->area==1&&area==2;
  // Retain the entrance transition while a blocked placement is retried.
  if(visit&&(!entering||Placed))visit->area=area;
  if(!Placed&&entering){
   for(int radius:{8,12,16}){for(auto& delta:AmazonUnity::Offsets){
    int x=Amazon::X(player)+delta[0]*radius/8,y=Amazon::Y(player)+delta[1]*radius/8;
    if(auto u=Spawn(game,player,x,y)){Id=Barbarian::Id(u);X=x;Y=y;Placed=true;HealAt=now+250;break;}
   }if(Placed)break;}
  }
  if(!Placed||area!=2)continue;
  auto u=Curses::Find(game,1,Id);
  if(now<HealAt)continue;
  if(Is(u)&&!AuraBuffs::Dead(u))Heal(u);
  else {
   if(Is(u)){if(auto room=SoulHarvest::Room(u))Barbarian::RoomDelete(room,1,Id);Barbarian::DeleteUnit(game,u);}
   Id=Invalid;if(auto fresh=Spawn(game,player,X,Y))Id=Barbarian::Id(fresh);
  }
  if(Id!=Invalid)HealAt=now+250;
 }
}
}
