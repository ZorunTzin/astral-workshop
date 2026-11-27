#pragma once
// Ephemeral formation: IDs only across frames, no saved pets or inventory edits.
namespace AmazonUnity {
#include "amazon_unity_bindings.h"
constexpr unsigned Invalid=0xffffffffu;
constexpr int Offsets[6][2]={{8,0},{4,7},{-4,7},{-8,0},{-4,-7},{4,-7}};
struct Formation {
 unsigned owner=Invalid,start=0,ready=0,lastShot=0;int x=0,y=0,level=1,shot=0;
 bool active=false,snapshot=false;int percent=10,flatMin=0,flatMax=0;Amazon::Impact damage{};
 std::array<League::Point,6> positions{};
 std::array<unsigned,6> actors{Invalid,Invalid,Invalid,Invalid,Invalid,Invalid};
};
inline std::array<Formation,16> Formations{};
inline void* Session=nullptr;inline unsigned Last=0;
inline bool Ghost(void* u){return u&&static_cast<unsigned*>(u)[0]==1&&static_cast<unsigned*>(u)[1]==Class;}
inline void Clear(void* game,Formation& f){
 for(auto& id:f.actors){if(id!=Invalid){auto u=Curses::Find(game,1,id);if(Ghost(u)){
  if(auto room=SoulHarvest::Room(u))Barbarian::RoomDelete(room,1,id);
  Barbarian::DeleteUnit(game,u);
 }}id=Invalid;}f.active=false;
}
inline void Reset(void* game){auto now=Ramp::Frame(game);if(Session!=game||now<Last){
 if(Session==game)for(auto& f:Formations)if(f.active)Clear(game,f);
 Formations={};Session=game;
 }Last=now;}
constexpr double TargetRadius=30.0; // Twenty yards from each stationary spirit.
inline void* NearestEnemy(void* game,void* spirit){
 void* best=nullptr;double distance=TargetRadius*TargetRadius;
 AuraFeatures::Units(game,1,[&](void* enemy){
  if(Ghost(enemy)||!GuardianCombat::Hostile(game,enemy)||!Barbarian::SameAuraArea(spirit,enemy))return;
  const double dx=Amazon::X(enemy)-Amazon::X(spirit),dy=Amazon::Y(enemy)-Amazon::Y(spirit),d=dx*dx+dy*dy;
  if(d<distance||(d==distance&&(!best||Barbarian::Id(enemy)<Barbarian::Id(best)))){best=enemy;distance=d;}
 });
 return best;
}
inline void ShootNext(void* game,void* owner,Formation& f){
 f.lastShot=Ramp::Frame(game);
 {unsigned index=unsigned(f.shot++)%6;auto u=Curses::Find(game,1,f.actors[index]);if(!Ghost(u))return;
  auto target=NearestEnemy(game,u);if(!target)return;
  const int tx=Amazon::X(target),ty=Amazon::Y(target);
  alignas(8) unsigned char mode[0x80]{};Barbarian::ModeInfo(u,4,mode);
  *reinterpret_cast<void**>(mode+0x10)=nullptr;
  *reinterpret_cast<int*>(mode+0x18)=tx;*reinterpret_cast<int*>(mode+0x1c)=ty;
  Barbarian::ModeChange(game,mode,0);
  Pulse::Params p{};p.flags=0x8021;p.owner=owner;p.origin=u;p.missile=Arrow;
  p.x=f.positions[index].x;p.y=f.positions[index].y;p.targetX=tx;p.targetY=ty;
  p.skill=Skill;p.level=f.level;p.range=int(std::ceil(std::hypot(double(tx-p.x),double(ty-p.y))*16/18))+1;
  auto m=Pulse::OnCreate(game,&p);if(!m)return;
  if(!f.snapshot){for(unsigned n=0;n<std::size(Amazon::DamageStats);++n)f.damage.damage[n]=Ramp::GetUnit(m,Amazon::DamageStats[n],0);f.snapshot=true;}
  Amazon::CopyDamage(m,f.damage,f.percent);
  // Native creation includes the flat skill bonus. Scale only the weapon
  // component, then retain the full synergy-adjusted flat bonus.
  // calc1/2 already return 8-bit fixed point, including half points.
  for(int i=0;i<2;++i){long long flat=i?f.flatMax:f.flatMin;
   auto value=std::max(0LL,1LL*f.damage.damage[i]-flat)*f.percent/100+flat;
   Ramp::SetUnit(m,21+i,int(std::min(value,0x3fffffffLL)),0);
  }
  // Argument one supplies the packet owner identity AND selects its recipients.
  // Using the player here still anchors client reconstruction at the player,
  // even with argument four set to the ghost. Monster publication includes
  // the player's room client; actual server missile ownership stays unchanged.
  if(Amazon::PublishMissiles){void* arrows[]={m};Amazon::PublishMissiles(u,arrows,1,u,nullptr,0);}
  if(f.shot<=6&&Ramp::Log){char line[240]{};std::snprintf(line,sizeof(line),"Unity arrow %u: server-owner=%u visual-owner=%u expected=(%d,%d) actual=(%d,%d) target=(%d,%d)",Barbarian::Id(m),Barbarian::Id(owner),Barbarian::Id(u),p.x,p.y,Amazon::X(m),Amazon::Y(m),tx,ty);Ramp::Log(line);}
 }
}
inline void Hit(void* g,void* a,void* d,void* p){
 if(Amazon::CurrentHitMissile&&int(static_cast<unsigned*>(Amazon::CurrentHitMissile)[1])==Arrow)Amazon::SplashHit(g,a,d,p,Arrow,Skill);
}
inline void Frame(void* game){
 if(!game)return;Reset(game);auto now=Ramp::Frame(game);
 for(auto& f:Formations){if(f.owner==Invalid)continue;auto owner=Curses::Find(game,0,f.owner);
  if(!owner){if(f.active)Clear(game,f);f={};continue;}
  if(!f.active)continue;
  bool valid=!AuraBuffs::Dead(owner)&&now-f.start<125;
  for(auto id:f.actors){auto u=Curses::Find(game,1,id);if(!Ghost(u)||!Barbarian::SameAuraArea(owner,u)){valid=false;break;}}
  if(!valid){Clear(game,f);continue;}
  // 60 sequential shots in 125 frames. Every sixth shot returns to
  // the same spirit (12/13 frames later); never emit twice in one frame.
  if(f.shot<60&&now>f.lastShot&&now-f.start>=unsigned((f.shot*25+11)/12))
   ShootNext(game,owner,f);
 }
}
inline int Cast(void* game,void* owner,int skill,int level){
 if(skill!=Skill)return -1;
 if(!game||!Curses::Player(owner)||static_cast<unsigned*>(owner)[1]!=0||AuraBuffs::Dead(owner)||level<1)return 0;
 if(!MonsterRuntime::Spawn||!League::RoomAt||!League::FreePosition||!Barbarian::RoomDelete||!Barbarian::DeleteUnit||!Barbarian::ModeInfo||!Barbarian::ModeChange||!GuardianCombat::ToggleState)return 0;
 Reset(game);auto stance=AmazonMasteries::player(game,owner);
 if(AmazonMasteries::weapon(owner)!=1||(stance&&stance->kind>=0&&stance->kind!=1))return 0;
 auto now=Ramp::Frame(game),id=Barbarian::Id(owner);Formation* slot=nullptr;
 for(auto& f:Formations)if(f.owner==id){slot=&f;break;}
 if(!slot)for(auto& f:Formations)if(f.owner==Invalid){slot=&f;break;}
 if(!slot||slot->active||now<slot->ready||Ramp::GetUnit(owner,8,0)<25*256)return 0;
 // Freeze the formation around the player's current position at cast time.
 const int x=Amazon::X(owner),y=Amazon::Y(owner);
 std::array<void*,6> rooms{};Formation fresh;
 for(int i=0;i<6;++i){
  const League::Point ideal{x+Offsets[i][0],y+Offsets[i][1]};bool placed=false;
  // Exact hexagon first, then bounded nearby tiles. The native search can
  // itself relocate a candidate and returns the room containing that tile.
  for(int radius=0;radius<=6&&!placed;++radius){
   for(int dx=-radius;dx<=radius&&!placed;++dx)for(int dy=-radius;dy<=radius&&!placed;++dy){
    if(std::max(std::abs(dx),std::abs(dy))!=radius)continue;
    League::Point found{ideal.x+dx,ideal.y+dy};
    auto room=League::RoomAt(SoulHarvest::Room(owner),found.x,found.y);if(!room)continue;
    room=League::FreePosition(room,&found,&found,1,0x3e01,0xc01,0);if(!room)continue;
    if(std::abs(found.x-ideal.x)>6||std::abs(found.y-ideal.y)>6)continue;
    bool separate=true;for(int j=0;j<i;++j){int sx=found.x-fresh.positions[j].x,sy=found.y-fresh.positions[j].y;if(sx*sx+sy*sy<4){separate=false;break;}}
    if(!separate)continue;
    rooms[i]=room;fresh.positions[i]=found;placed=true;
   }
  }
  if(!placed)return 0;
 }
 fresh.percent=std::max(0,Curses::EvalCalc4(game,owner,Skill,level,9+level));
 fresh.flatMin=Sorceress::AddedDamage(game,owner,Skill,level,false);
 fresh.flatMax=Sorceress::AddedDamage(game,owner,Skill,level,true);
 fresh.owner=id;fresh.x=x;fresh.y=y;fresh.level=std::clamp(level,1,1000);fresh.start=now;
 for(int i=0;i<6;++i){int ax=fresh.positions[i].x,ay=fresh.positions[i].y;auto u=MonsterRuntime::Spawn(game,rooms[i],ax,ay,Class,1,-1,0);
  if(u)fresh.actors[i]=Barbarian::Id(u);
  if(!Ghost(u)||Amazon::X(u)!=ax||Amazon::Y(u)!=ay||!Barbarian::SameAuraArea(owner,u)){Clear(game,fresh);return 0;}
  Ramp::SetUnit(u,172,1,0);GuardianCombat::ToggleState(u,State,1);
  if(SoulHarvest::Refresh)SoulHarvest::Refresh(u);
  if(SoulHarvest::MarkGfxDirty)SoulHarvest::MarkGfxDirty(u,State,1);
  Curses::Update(u,"Amazonian Unity spirit",0);
 }
 fresh.active=true;fresh.ready=now+250;*slot=fresh;
 Ramp::SetUnit(owner,8,Ramp::GetUnit(owner,8,0)-25*256,0);
 ShootNext(game,owner,*slot);return 1;
}
}
