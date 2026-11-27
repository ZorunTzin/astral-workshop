#pragma once
#include <cmath>
// All scheduled work keeps IDs and values, never pointers across game frames.
namespace Amazon {
#include "amazon_bindings.h"
#include "amazon_bow_bindings.h"
constexpr unsigned Invalid=0xffffffffU;
inline Curses::MissileHitFn OriginalHit=nullptr;
inline thread_local void* CurrentHitMissile=nullptr;
// Native volley publisher handles the owning client and other room viewers.
// Use the spawn packet path, not the late-room resynchronization helper.
using PublishFn=void(__fastcall*)(void*,void**,unsigned char,void*,void*,unsigned short);
inline PublishFn PublishMissiles=nullptr;
inline void* SpawnedMissile=nullptr; // Valid only during a synchronous child cast.

inline void* Session=nullptr;inline unsigned LastFrame=0,Depth=0;
constexpr int DamageStats[]={21,22,48,49,50,51,52,53,54,55,56,57,58,59,316,317};
struct Flight {unsigned id=Invalid,owner=Invalid,cls=0,until=0,lastTarget=Invalid;int level=1,stage=-1;bool handled=false,published=false,scaled=false;};
struct Impact {unsigned owner=Invalid,missile=Invalid,cls=0,target=Invalid,due=0;int level=1,stage=-1,x=0,y=0;bool returning=false;std::array<int,std::size(DamageStats)> damage{};};
inline std::array<Impact,4096> Snapshots{};
inline const Impact* SpawnContext=nullptr;inline int SpawnClass=0;
inline void CopyDamage(void*,const Impact&,int);
inline void PublishFlight(void*,void*);
inline void BeforeTick(void*,void*) noexcept;
inline std::array<std::array<unsigned,8>,2048> DiagnosticLogs{};
inline void Trace(const char* event,unsigned id,int cls,int detail){if(Ramp::Log&&DiagnosticLogs[std::clamp(cls,0,2047)][event[0]=='c'?0:event[0]=='f'?1:event[0]=='r'?2:event[0]=='s'?3:event[0]=='v'?4:5]++<12){char b[180]{};std::snprintf(b,sizeof(b),"Amazon projectile: %s id=%u class=%d detail=%d",event,id,cls,detail);Ramp::Log(b);}}

struct Fan {unsigned owner=Invalid,frame=0;int skill=0,index=0;};
inline std::array<Flight,4096> Flights{};inline std::array<Impact,1024> Pending{};inline std::array<Fan,32> Fans{};
inline bool Queue(const Impact& value){for(auto& j:Pending)if(j.owner==Invalid){j=value;return true;}return false;}
inline int Level(int l){return std::clamp(l,1,1000);}
inline int ChargedCount(int l){return 3+Level(l)/5;}
inline int ChainCount(int l){return 1+Level(l)/10;}
inline int ToxicCount(int l){return 8+Level(l);}
inline int ToxicVelocity(int l){return int(16LL*256*(100+5*Level(l))/100);}
// Return uses Vel=24, no level velocity. Native server creation applies 75%,
// giving 18 path units/frame, with 16 path units per world tile. The native
// distance approximation overestimates diagonal journeys and overshoots the
// caster. Floor the Euclidean flight time so the last step cannot pass them.
inline int ReturnFrames(int dx,int dy){return std::max(1,int(std::floor(std::hypot(double(dx),double(dy))*16.0/18.0)));}

inline int Stage(int cls){return cls==Split0?0:cls==Split1?1:cls==Split2?2:cls==Split3?3:cls==Split4?4:-1;}
inline int X(void* u){return Assassin::Coordinate(u,false);}inline int Y(void* u){return Assassin::Coordinate(u,true);}
inline void Reset(void* game){unsigned frame=Ramp::Frame(game);if(Session!=game||frame<LastFrame){Session=game;Flights={};Pending={};Snapshots={};Fans={};Depth=0;DiagnosticLogs={};}LastFrame=frame;}
inline bool BuffPulse(void* g,void* u,unsigned i,int level){
 const int sid=AuraBuffs::Definitions[i].skill;if(sid!=8&&sid!=17&&sid!=18&&sid!=28)return false;
 const int state=AuraBuffs::Definitions[i].state;
 if(sid==8||sid==17){
  auto* list=Ramp::OriginalList(u,state);
  if(!list){Curses::Curse r{u,u,sid,level,75,-1,0,state,nullptr};list=Curses::OriginalApply(&r);}
  if(list){Ramp::SetExpiry(list,int(Ramp::Frame(g)+75));Ramp::Schedule(g,u,12,int(Ramp::Frame(g)+75),0,0,0);}
 }
 AuraBuffs::Native[i](g,u,sid,level);return true;
}
inline bool Filter(void* game,void* raw)noexcept{
 auto* p=static_cast<Pulse::Params*>(raw);if(!p||!Curses::Player(p->owner))return true;
 if(p->missile!=ChargedBolt&&p->missile!=Toxic)return true;
 Reset(game);Fan* f=nullptr;unsigned owner=Barbarian::Id(p->owner),frame=Ramp::Frame(game);
 for(auto& a:Fans)if(a.owner==owner&&a.skill==p->skill){f=&a;break;}
 if(!f)for(auto& a:Fans)if(a.owner==Invalid||a.frame!=frame){f=&a;a={owner,frame,p->skill,0};break;}
 if(!f)return true;if(f->frame!=frame){f->frame=frame;f->index=0;}
 const int count=p->missile==ChargedBolt?ChargedCount(p->level):ToxicCount(p->level);
 double angle=6.283185307179586*(f->index++%count)/count;
 p->flags=(p->flags|0x21U)&~2U;p->target=nullptr;p->origin=p->owner;p->x=X(p->owner);p->y=Y(p->owner);
 // Remove Charged Strike's fan initializer after supplying our own direction.
 // The client table uses its nearest integer VelLev step; server travel uses
 // the exact requested 5% per level without changing poison damage level.
 if(p->missile==Toxic){p->flags|=0x14;p->velocity=ToxicVelocity(p->level);}
 p->initializeFunction=0;p->initializeArgument=0;
 p->targetX=p->x+int(std::lround(30*std::cos(angle)));p->targetY=p->y+int(std::lround(30*std::sin(angle)));

 return true;
}
inline void Created(void* game,void* raw,void* missile)noexcept{
 auto* p=static_cast<Pulse::Params*>(raw);if(!p||!missile||!Curses::Player(p->owner))return;Reset(game);
 if(p->missile==MultiArrow||p->missile==MultiBolt||Stage(p->missile)>=0){
  const unsigned id=Barbarian::Id(missile),slot=id%Flights.size();
  auto& f=Flights[slot];
  f={id,Barbarian::Id(p->owner),unsigned(p->missile),Ramp::Frame(game)+300,Invalid,Level(p->level),Stage(p->missile),false};
  // All split stages are published explicitly after native initialization.
  auto& snap=Snapshots[slot];snap={f.owner,id,f.cls,Invalid,0,f.level,f.stage,X(missile),Y(missile),f.stage<0,{}};
  for(unsigned n=0;n<std::size(DamageStats);++n)snap.damage[n]=Ramp::GetUnit(missile,DamageStats[n],0);
  Trace("created",id,p->missile,p->level);

 }
 if(SpawnContext&&Stage(p->missile)>=0){CopyDamage(missile,*SpawnContext,100);SpawnedMissile=missile;}
 if(p->missile==ChargedBolt||p->missile==Toxic)Trace("radial created",Barbarian::Id(missile),p->missile,p->level);
 if(p->missile==PounceHit){
  const int percent=Curses::EvalCalc4(game,p->owner,19,p->level,100);
  for(int stat:DamageStats)Ramp::SetUnit(missile,stat,(stat==21||stat==22)?int(std::min<long long>(0x3fffffff,1LL*Ramp::GetUnit(missile,stat,0)*percent/100)):0,0);
 }
 // Native chain descendants are created inside OriginalHit and must not
 // multiply again. Only the initial melee release fans into extra chains.
 if(p->missile==LightningStrike&&p->skill==34&&!Depth){
  Trace("chain volley",Barbarian::Id(missile),LightningStrike,ChainCount(p->level));
  ++Depth;for(int n=1;n<ChainCount(p->level);++n){auto extra=*p;Pulse::OnCreate(game,&extra);}--Depth;
 }
}
inline int __fastcall MissileHit(void* game,void* missile,void* target,int force){
 struct Context {void* prior;Context(void* m):prior(CurrentHitMissile){CurrentHitMissile=m;}~Context(){CurrentHitMissile=prior;}} context(missile);
 Reset(game);BeforeTick(game,missile);Impact request;bool queued=false;
 const int cls=missile?static_cast<unsigned*>(missile)[1]:-1;
 if(cls==Return&&target&&GuardianCombat::Hostile(game,target))Trace("damage at return collision",Barbarian::Id(missile),cls,Ramp::GetUnit(missile,21,0));
 if(!Depth&&Stage(cls)>=0&&target&&GuardianCombat::Hostile(game,target)){
  for(auto& f:Flights)if(f.id==Barbarian::Id(missile)&&f.cls==static_cast<unsigned*>(missile)[1]&&f.until>Ramp::Frame(game)){
   if(!f.handled&&f.lastTarget!=Barbarian::Id(target)){
    f.lastTarget=Barbarian::Id(target);f.handled=true;
    request={f.owner,f.id,f.cls,f.lastTarget,Ramp::Frame(game)+1,f.level,f.stage,X(target),Y(target),f.stage<0,{}};
    for(unsigned i=0;i<std::size(DamageStats);++i)request.damage[i]=Ramp::GetUnit(missile,DamageStats[i],0);
    queued=f.stage<4;
   }break;
  }
 }
 ++Depth;int result=OriginalHit(game,missile,target,force);--Depth;
 if(queued){Queue(request);Trace("split impact",request.missile,cls,request.stage);}
 return result;
}
inline void PublishVisual(void* game,void* owner,void* missile,void* target=nullptr){
 if(!game||!missile||!Curses::Player(owner)||!PublishMissiles)return;
 // No prediction token and no caster-origin override: preserve the actual
 // remote spawn position/path. Native code enumerates each client once.
 void* arrows[]={missile};const bool split=Stage(static_cast<unsigned*>(missile)[1])>=0;
 void* anchor=split?missile:owner;PublishMissiles(anchor,arrows,1,split?missile:nullptr,target,0);
 const auto id=Barbarian::Id(missile);auto& f=Flights[id%Flights.size()];
 if(f.id==id&&f.cls==static_cast<unsigned*>(missile)[1])f.published=true;
 Trace("volley published",id,int(static_cast<unsigned*>(missile)[1]),1);
}
// Only publish after the native skill has finished initializing its missile.
// Client prediction is disabled for these two skills; this is the sole initial
// visual, shared with the damage-bearing server arrow, not a second shot.
inline void PublishFlight(void* game,void* missile){
 if(!missile)return;const auto id=Barbarian::Id(missile);auto& f=Flights[id%Flights.size()];
 if(f.id!=id||f.cls!=static_cast<unsigned*>(missile)[1]||f.published||f.until<=Ramp::Frame(game))return;
 if(void* owner=Curses::Find(game,0,f.owner))PublishVisual(game,owner,missile);
}
inline void CopyDamage(void* missile,const Impact& j,int percent){
 for(unsigned i=0;i<std::size(DamageStats);++i){int stat=DamageStats[i];int value=j.damage[i];if(stat!=56&&stat!=59)value=int(1LL*value*percent/100);Ramp::SetUnit(missile,stat,value,0);}
}
inline void Shoot(void* game,void* owner,void* target,const Impact& j,int cls,int skill,int percent){
 // Children are ordinary arrows created at the impact, aimed at the
 // selected enemy. No replay of the player skill or Guided Arrow state.
 Pulse::Params p{};p.flags=0x21;p.owner=owner;p.origin=owner;p.target=nullptr;p.missile=cls;p.skill=skill;p.level=j.level;
 p.x=j.x;p.y=j.y;p.targetX=X(target);p.targetY=Y(target);
 // Start just outside the impact center, toward the new target, so a child
 // does not instantly collide with the same victim before it can move.
 const double dx=p.targetX-p.x,dy=p.targetY-p.y,dist=std::sqrt(dx*dx+dy*dy);
 if(dist>0){p.x+=int(std::lround(dx/dist));p.y+=int(std::lround(dy/dist));}
 if(cls==Return){p.flags|=0x8000;p.range=ReturnFrames(p.targetX-p.x,p.targetY-p.y);}
 if(void* shot=Pulse::OnCreate(game,&p)){CopyDamage(shot,j,percent);Trace("payload return",Barbarian::Id(shot),cls,Ramp::GetUnit(shot,21,0));PublishVisual(game,owner,shot);}
}
inline void Frame(void* game){
 Reset(game);const auto now=Ramp::Frame(game);
 // Guided Arrow has a distinct movement handler. The frame fallback covers
 // it without depending on the generic missile-update hook being called.
 for(auto& f:Flights)if(f.id!=Invalid&&!f.published&&!f.handled&&f.until>now){
  if(void* missile=Curses::Find(game,3,f.id))BeforeTick(game,missile);
 }
 for(auto& pending:Pending){
  if(pending.owner==Invalid||now<pending.due)continue;Impact j=pending;pending={};
  auto* owner=Curses::Find(game,0,j.owner);if(!AuraFeatures::Alive(owner))continue;
  if(j.returning){
   Shoot(game,owner,owner,j,Return,12,50);
   Trace("return",j.missile,j.cls,50);
  }else{
   void* nearest=nullptr;double distance=1e20;
   AuraFeatures::Units(game,1,[&](void* enemy){if(GuardianCombat::Hostile(game,enemy)&&Barbarian::SameAuraArea(owner,enemy)&&Barbarian::Id(enemy)!=j.target){double dx=X(enemy)-j.x,dy=Y(enemy)-j.y,d=dx*dx+dy*dy;if(d<distance&&d<=225){distance=d;nearest=enemy;}}});
   if(nearest&&j.stage>=0&&j.stage<4){const int ids[]={Split0,Split1,Split2,Split3,Split4};for(int n=0;n<2;++n)Shoot(game,owner,nearest,j,ids[j.stage+1],22,100);}
  }
 }
}
// Called before native movement. FinishTick receives only an ID and result;
// it must never read the potentially removed missile after native processing.
inline void BeforeTick(void* game,void* missile) noexcept {
 if(!missile)return;Reset(game);const unsigned id=Barbarian::Id(missile),slot=id%Flights.size();auto& f=Flights[slot];
 if(f.id!=id||f.until<=Ramp::Frame(game)||f.handled)return;
 PublishFlight(game,missile);
 if(f.stage==0&&!f.scaled){
  // Native fan creation finishes damage initialization after AfterCreate.
  // Apply the old skill bonus once; descendants inherit the resulting stats.
  if(void* owner=Curses::Find(game,0,f.owner)){
   int bonus=Curses::EvalCalc4(game,owner,22,f.level,0);
   for(int stat:{21,22})Ramp::SetUnit(missile,stat,int(std::clamp<long long>(1LL*Ramp::GetUnit(missile,stat,0)*(100LL+bonus)/100,0,0x3fffffff)),0);
  }
  // Scale the finalized payload once, including added elemental damage.
  // Cold/poison duration remains unchanged; descendants inherit this payload.
  for(int stat:DamageStats)if(stat!=56&&stat!=59)Ramp::SetUnit(missile,stat,Ramp::GetUnit(missile,stat,0)/3,0);
  f.scaled=true;
 }
 if(f.stage>=0)return;
 auto& snap=Snapshots[slot];snap.x=X(missile);snap.y=Y(missile);snap.due=Ramp::Frame(game)+1;
 // Multiple Shot may adjust damage after the creation callback. Snapshot
 // the fully initialized outgoing arrow, not its provisional creation stats.
 for(unsigned n=0;n<std::size(DamageStats);++n)snap.damage[n]=Ramp::GetUnit(missile,DamageStats[n],0);
}
inline void FinishTick(void* game,unsigned id,int result) noexcept {
 auto& f=Flights[id%Flights.size()];
 if(result!=2||f.id!=id||f.handled||f.stage>=0||f.until<=Ramp::Frame(game))return;
 f.handled=true;Queue(Snapshots[id%Flights.size()]);Trace("flight ended",id,f.cls,result);
}
// A successful Strafe impact splashes its finalized missile payload once.
// Snapshot target IDs before native damage (which can remove units). The
// primary victim is excluded; misses, blocks and splash hits cannot recurse.
inline unsigned SplashDepth=0;
constexpr double StrafeRadius=6.0; // Four yards.
inline bool SplashImpact(void* attacker,void* packet,int extra=-1){
 if(SplashDepth||Curses::ProcDepth||!packet||!Curses::Player(attacker)||!CurrentHitMissile)return false;
 const unsigned cls=static_cast<unsigned*>(CurrentHitMissile)[1];
 if(cls!=StrafeArrow&&cls!=StrafeBolt&&int(cls)!=extra)return false;
 unsigned short flags=0;std::memcpy(&flags,static_cast<unsigned char*>(packet)+4,2);
 return (flags&1)&&!(flags&0x4020)&&Barbarian::ReadDamage(packet,0x134)>0;
}
inline bool StrafeImpact(void* a,void* p){return SplashImpact(a,p);}
inline void SplashHit(void* game,void* attacker,void* defender,void* packet,int extra=-1,int skill=26){
 if(!SplashImpact(attacker,packet,extra)||!GuardianCombat::Hostile(game,defender))return;
 Impact payload;for(unsigned i=0;i<std::size(DamageStats);++i)payload.damage[i]=Ramp::GetUnit(CurrentHitMissile,DamageStats[i],0);
 const int x=X(defender),y=Y(defender);const unsigned primary=Barbarian::Id(defender);
 std::vector<unsigned> targets;
 AuraFeatures::Units(game,1,[&](void* enemy){
  const double dx=X(enemy)-x,dy=Y(enemy)-y;
  if(Barbarian::Id(enemy)!=primary&&GuardianCombat::Hostile(game,enemy)&&Barbarian::SameAuraArea(attacker,enemy)&&dx*dx+dy*dy<=StrafeRadius*StrafeRadius)targets.push_back(Barbarian::Id(enemy));
 });
 ++SplashDepth;++Curses::ProcDepth;
 for(auto id:targets)if(void* enemy=Curses::Find(game,1,id)){
  if(!GuardianCombat::Hostile(game,enemy)||!Barbarian::SameAuraArea(attacker,enemy))continue;
  Pulse::Params p{};p.flags=0x8021;p.owner=attacker;p.origin=enemy;p.target=enemy;p.missile=StrafeSplash;p.skill=skill;p.level=1;p.x=X(enemy)-1;p.y=Y(enemy);p.targetX=p.x+2;p.targetY=p.y;p.range=1;
  if(void* missile=Pulse::OnCreate(game,&p)){CopyDamage(missile,payload,50);Curses::NativeMissileHit(game,missile,enemy,0);}
 }
 --Curses::ProcDepth;--SplashDepth;
}
inline void Hit(void* g,void* a,void* d,void* p){SplashHit(g,a,d,p);}
// Same relocation service used by native Dragon Flight (srvdofunc 52).
// Pass explicit CURRENT target coordinates; item-proc Teleport instead resolves
// the caster's cursor/path destination and may jump away from the victim.
using RelocateFn=void(__fastcall*)(void*,void*,void*,int,int,int,int);
inline RelocateFn Relocate=nullptr;
inline bool LandAtTarget(void* game,void* attacker,void* target){
 if(!Relocate||!GuardianCombat::Hostile(game,target)||!Barbarian::SameAuraArea(attacker,target))return false;
 const int tx=X(target),ty=Y(target);
 Relocate(game,attacker,nullptr,tx,ty,0,0);
 const double dx=X(attacker)-tx,dy=Y(attacker)-ty;
 return dx*dx+dy*dy<=9;
}
constexpr double PounceRadius=9.0; // Six displayed yards (1.5 native subtiles/yard).
constexpr double PounceSearchRadius=18.0; // Ground-click acquisition: twelve yards.
inline void ConsiderPounceTarget(void* game,void* attacker,void* candidate,void*& best,double& distance){
 if(!GuardianCombat::Hostile(game,candidate)||!Barbarian::SameAuraArea(attacker,candidate))return;
 const double dx=X(candidate)-X(attacker),dy=Y(candidate)-Y(attacker),d=dx*dx+dy*dy;
 if(d<distance||(d==distance&&(!best||Barbarian::Id(candidate)<Barbarian::Id(best)))){best=candidate;distance=d;}
}
inline void* PounceTarget(void* game,void* attacker){
 void* target=Assassin::GetTarget(game,attacker);
 if(GuardianCombat::Hostile(game,target)&&Barbarian::SameAuraArea(attacker,target))return target;
 void* best=nullptr;double distance=PounceSearchRadius*PounceSearchRadius;
 AuraFeatures::Units(game,1,[&](void* enemy){ConsiderPounceTarget(game,attacker,enemy,best,distance);});
 return best;
}
inline int Cast(void* game,void* attacker,int skill,int level){
 if(skill!=19)return -1;Reset(game);
 if(Depth||!Curses::Player(attacker)||!Assassin::GetTarget)return 0;
 auto* target=PounceTarget(game,attacker);
 if(!target)return 0;
 const unsigned targetId=Barbarian::Id(target);
 ++Depth;
 const bool moved=LandAtTarget(game,attacker,target);
 target=Curses::Find(game,1,targetId);
 if(!moved||!target){--Depth;return 0;}
 const double tx=X(target)-X(attacker),ty=Y(target)-Y(attacker);
 if(tx*tx+ty*ty>36){--Depth;return 0;}
 Curses::Cast(attacker,PounceVisual,level,attacker,0);
 std::array<unsigned,256> ids{};unsigned count=0;
 AuraFeatures::Units(game,1,[&](void* enemy){double dx=X(enemy)-X(attacker),dy=Y(enemy)-Y(attacker);if(count<ids.size()&&GuardianCombat::Hostile(game,enemy)&&Barbarian::SameAuraArea(attacker,enemy)&&dx*dx+dy*dy<=PounceRadius*PounceRadius)ids[count++]=Barbarian::Id(enemy);});
 for(unsigned n=0;n<count;++n)if(void* enemy=Curses::Find(game,1,ids[n])){
  Pulse::Params p{};p.flags=0x8021;p.owner=attacker;p.origin=enemy;p.target=enemy;p.missile=PounceHit;p.skill=19;p.level=level;p.x=X(enemy)-1;p.y=Y(enemy);p.targetX=p.x+2;p.targetY=p.y;p.range=1;
  if(void* m=Pulse::OnCreate(game,&p))Curses::NativeMissileHit(game,m,enemy,0);
 }
 Trace("pounce",Barbarian::Id(attacker),19,int(count));--Depth;return 1;
}
inline bool Install(const D2RL::PluginContext* context){
 const unsigned char relocateBytes[]={0x40,0x55,0x53,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57};
 if(!context->api->checkExpectedBytes(context,0x491ca0,relocateBytes,sizeof(relocateBytes)))return false;
 Relocate=reinterpret_cast<RelocateFn>(context->exeBase+0x491ca0);

 // Native volley signature verified at call sites 0x462db6 and 0x462e07.
 const unsigned char publishBytes[]={0x45,0x84,0xc0,0x0f,0x84,0x0e,0x05,0x00,0x00};
 if(!context->api->checkExpectedBytes(context,0x4640f0,publishBytes,sizeof(publishBytes)))return false;
 PublishMissiles=reinterpret_cast<PublishFn>(context->exeBase+0x4640f0);

 if(!context->api->checkExpectedBytes(context,CurseNative::MissileHitRva,CurseNative::MissileHitBytes,sizeof(CurseNative::MissileHitBytes)))return false;
 auto hook=D2RL::MakeInlineHook(CurseNative::MissileHitRva,CurseNative::MissileHitBytes,sizeof(CurseNative::MissileHitBytes),reinterpret_cast<void*>(MissileHit),reinterpret_cast<void**>(&OriginalHit));
 return context->api->installInlineHook(context,&hook);
}
}
