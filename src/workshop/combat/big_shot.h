#pragma once
namespace BigShot {
#include "big_shot_bindings.h"
constexpr unsigned Invalid=0xffffffffu;
constexpr int ManaCost(int level){return 10*256+std::max(0,level-1)*128;}
constexpr double Width=4.0,Radius=9.0;
struct Flight {
 unsigned owner=Invalid,missile=Invalid,start=0,last=Invalid;
 int x=0,y=0,level=1,pulses=0;bool active=false;
 Amazon::Impact damage{};std::vector<unsigned> struck;
};
inline std::array<Flight,256> Flights{};inline void* Game=nullptr;inline unsigned Last=0;
inline bool Carrier(void* u){return u&&static_cast<unsigned*>(u)[0]==3&&int(static_cast<unsigned*>(u)[1])==Arrow;}
// Client handler 1 decrements lifetime, but only self-removes units with
// UNITFLAG_ISASYNC (unit +0x124 bit 21). Explicit server volley visuals
// must opt into that expiry path; server ownership/damage remain unchanged.
using ClientTickFn=void(__fastcall*)(void*,float);
inline ClientTickFn OriginalClientTick=nullptr;
inline void __fastcall ClientTick(void* missile,float elapsed){
 if(Carrier(missile)){
  unsigned flags=0;auto p=static_cast<unsigned char*>(missile)+0x124;
  std::memcpy(&flags,p,4);flags|=0x00200000u;std::memcpy(p,&flags,4);
 }
 // Native expiry can free the unit. Never read it after this call.
 OriginalClientTick(missile,elapsed);
}
inline bool Install(const D2RL::PluginContext* context){
 const unsigned char expected[]={0x40,0x53,0x55,0x57,0x48,0x81,0xec,0x90,0,0,0,0x0f,0x29,0x74,0x24,0x60};
 if(!context->api->checkExpectedBytes(context,0x1b5380,expected,sizeof(expected)))return false;
 auto hook=D2RL::MakeInlineHook(0x1b5380,expected,sizeof(expected),reinterpret_cast<void*>(ClientTick),reinterpret_cast<void**>(&OriginalClientTick));
 return context->api->installInlineHook(context,&hook);
}
inline void Clear(void* game,Flight& f){
 auto m=Curses::Find(game,3,f.missile);
 if(Carrier(m)){if(auto room=SoulHarvest::Room(m))Barbarian::RoomDelete(room,3,f.missile);Barbarian::DeleteUnit(game,m);}
 f.active=false;f.missile=Invalid;f.struck.clear();
}
inline void Reset(void* game){auto now=Ramp::Frame(game);if(Game!=game||now<Last){Flights={};Game=game;}Last=now;}
inline double SegmentDistance(int x,int y,int ax,int ay,int bx,int by){
 double dx=bx-ax,dy=by-ay,den=dx*dx+dy*dy;
 double t=den?std::clamp(((x-ax)*dx+(y-ay)*dy)/den,0.0,1.0):0;
 dx=x-(ax+t*dx);dy=y-(ay+t*dy);return dx*dx+dy*dy;
}
inline void Damage(void* game,void* owner,void* target,const Flight& f,int percent){
 if(!GuardianCombat::Hostile(game,target)||!Barbarian::SameAuraArea(owner,target))return;
 Pulse::Params p{};p.flags=0x8021;p.owner=owner;p.origin=target;p.target=target;p.missile=HitMissile;
 p.skill=Skill;p.level=f.level;p.x=Amazon::X(target)-1;p.y=Amazon::Y(target);p.targetX=p.x+2;p.targetY=p.y;p.range=1;
 if(auto m=Pulse::OnCreate(game,&p)){Amazon::CopyDamage(m,f.damage,percent);Curses::NativeMissileHit(game,m,target,0);}
}
inline void NovaVisual(void* game,void* owner,void* carrier,const Flight& f,int x,int y){
 constexpr int dirs[16][2]={{100,0},{92,38},{71,71},{38,92},{0,100},{-38,92},{-71,71},{-92,38},{-100,0},{-92,-38},{-71,-71},{-38,-92},{0,-100},{38,-92},{71,-71},{92,-38}};
 for(auto& d:dirs){Pulse::Params p{};p.flags=0x8021;p.owner=owner;p.origin=carrier;p.missile=Nova;p.skill=Skill;p.level=f.level;
  p.x=x;p.y=y;p.targetX=x+d[0];p.targetY=y+d[1];p.range=8;
  if(auto m=Pulse::OnCreate(game,&p))if(Amazon::PublishMissiles){void* shots[]={m};Amazon::PublishMissiles(carrier,shots,1,carrier,nullptr,0);}
 }
}
inline void Tick(void* game,void* owner,void* carrier,Flight& f,unsigned now){
 if(f.last==now)return;f.last=now;
 int x=Amazon::X(carrier),y=Amazon::Y(carrier);
 const bool nova=f.pulses<10&&now-f.start>=unsigned((f.pulses*25+24)/2);
 std::vector<unsigned> direct,area;
 AuraFeatures::Units(game,1,[&](void* enemy){
  if(!GuardianCombat::Hostile(game,enemy)||!Barbarian::SameAuraArea(owner,enemy))return;
  const auto id=Barbarian::Id(enemy);const int ex=Amazon::X(enemy),ey=Amazon::Y(enemy);
  if(std::find(f.struck.begin(),f.struck.end(),id)==f.struck.end()&&SegmentDistance(ex,ey,f.x,f.y,x,y)<=Width*Width){direct.push_back(id);f.struck.push_back(id);}
  double dx=ex-x,dy=ey-y;if(nova&&dx*dx+dy*dy<=Radius*Radius)area.push_back(id);
 });
 f.x=x;f.y=y;
 for(auto id:direct)if(auto u=Curses::Find(game,1,id))Damage(game,owner,u,f,100);
 if(nova){++f.pulses;NovaVisual(game,owner,carrier,f,x,y);for(auto id:area)if(auto u=Curses::Find(game,1,id))Damage(game,owner,u,f,50);}
}
inline void Frame(void* game){
 if(!game)return;Reset(game);auto now=Ramp::Frame(game);
 for(auto& f:Flights){if(f.owner==Invalid)continue;auto owner=Curses::Find(game,0,f.owner);
  if(!owner){if(f.active)Clear(game,f);f={};continue;}
  if(!f.active)continue;auto m=Curses::Find(game,3,f.missile);
  if(AuraBuffs::Dead(owner)||!Carrier(m)||!Barbarian::SameAuraArea(owner,m)||now-f.start>=125){Clear(game,f);continue;}
  Tick(game,owner,m,f,now);
 }
}
inline int Cast(void* game,void* owner,int skill,int level){
 if(skill!=Skill)return -1;
 if(!game||!Curses::Player(owner)||static_cast<unsigned*>(owner)[1]!=0||AuraBuffs::Dead(owner)||level<1||!Pulse::OriginalCreate||!Curses::NativeMissileHit||!Barbarian::DeleteUnit||!Barbarian::RoomDelete)return 0;
 if(AmazonMasteries::weapon(owner)!=1)return 0;
 Reset(game);auto id=Barbarian::Id(owner),now=Ramp::Frame(game);Flight* slot=nullptr;
 // Each cast owns its own flight; an existing arrow never blocks recasts.
 for(auto& f:Flights)if(!f.active){slot=&f;break;}
 if(!slot||Ramp::GetUnit(owner,8,0)<ManaCost(level))return 0;
 const int x=Amazon::X(owner),y=Amazon::Y(owner);int tx=Curses::X(owner),ty=Curses::Y(owner);
 if(auto target=Assassin::GetTarget?Assassin::GetTarget(game,owner):nullptr){tx=Amazon::X(target);ty=Amazon::Y(target);}
 const double dx=tx-x,dy=ty-y,length=std::hypot(dx,dy);if(length<1)return 0;
 Pulse::Params p{};p.flags=0x8021;p.owner=owner;p.origin=owner;p.missile=Arrow;p.skill=Skill;p.level=level;
 p.x=x;p.y=y;p.targetX=x+int(std::lround(dx*100/length));p.targetY=y+int(std::lround(dy*100/length));p.range=125;
 auto m=Pulse::OnCreate(game,&p);if(!Carrier(m))return 0;
 Flight fresh;fresh.owner=id;fresh.missile=Barbarian::Id(m);fresh.start=now;fresh.level=level;fresh.x=x;fresh.y=y;fresh.active=true;
 Amazon::Impact raw;for(unsigned i=0;i<std::size(Amazon::DamageStats);++i)raw.damage[i]=Ramp::GetUnit(m,Amazon::DamageStats[i],0);
 // calc3 retains the original percentage numerator. Divide the actual
 // fixed-point damage by 300, avoiding rounding 33 1/3% down to 33%.
 int numerator=95+5*level;
 if(auto row=Curses::Record(Ramp::Version(game),Skill)){int formula=-1;std::memcpy(&formula,static_cast<unsigned char*>(row)+0x198,4);if(formula>=0)numerator=Curses::Evaluate(Ramp::Version(game),owner,formula,Skill,level);}
 numerator=std::max(0,numerator);
 for(unsigned i=0;i<std::size(Amazon::DamageStats);++i){int stat=Amazon::DamageStats[i];auto value=(stat==56||stat==59)?raw.damage[i]:1LL*raw.damage[i]*numerator/300;Ramp::SetUnit(m,stat,int(std::min(value,0x3fffffffLL)),0);}
 // Same fixed-point flat bonus as Unity; scale only the weapon component.
 for(int i=0;i<2;++i){long long flat=Sorceress::AddedDamage(game,owner,Skill,level,i!=0);
  auto damage=std::max(0LL,1LL*raw.damage[i]-flat)*numerator/300+flat;
  Ramp::SetUnit(m,21+i,int(std::min(damage,0x3fffffffLL)),0);
 }
 for(unsigned i=0;i<std::size(Amazon::DamageStats);++i)fresh.damage.damage[i]=Ramp::GetUnit(m,Amazon::DamageStats[i],0);
 *slot=std::move(fresh);Ramp::SetUnit(owner,8,Ramp::GetUnit(owner,8,0)-ManaCost(level),0);
 Amazon::PublishVisual(game,owner,m);return 1;
}
}
