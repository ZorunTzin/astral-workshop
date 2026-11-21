#pragma once
#include "../assassin_preview/engine.h"
// Native bridge: only typed IDs and coordinates persist between server frames.
namespace Assassin {
#include "assassin_bindings.h"
using namespace AssassinPreview;
inline void* Game=nullptr;
inline unsigned LastFrame=0,Depth=0,Random=0x1938ba72;
inline const Hit* ActiveHit=nullptr;
using AllocateFn=void(__fastcall*)(void*,void*,void*,int,int);
using ReleaseFn=void(__fastcall*)(void*,void*,int,void*);
using SetTargetFn=void(__fastcall*)(void*,void*);
using GetTargetFn=void*(__fastcall*)(void*,void*);
inline AllocateFn Allocate=nullptr;
inline ReleaseFn Release=nullptr;
inline Curses::DamageForFn DamageFor=nullptr;
inline Curses::MeleeFn ResolveMelee=nullptr;
inline SetTargetFn SetTarget=nullptr;
inline GetTargetFn GetTarget=nullptr;
inline unsigned HitLogs=0;
inline void* VisualOrigin=nullptr;
inline bool ChargeReleaseDone=false;
struct Pet {Id id=Invalid,owner=Invalid;unsigned cls=0,born=0,last=0,next=0,credit=0;int shots=80,level=1;};
inline std::array<Pet,512> Pets{};

inline std::array<std::pair<Id,int>,16> Areas{};
inline int Public(Skill s){switch(s){case Skill::Queen:return 277;case Skill::ATMG:return 266;case Skill::Spikes:return 257;case Skill::Afterimage:return 263;case Skill::DeathMark:return 278;case Skill::Gravity:return 256;case Skill::Overclock:return 276;case Skill::Omnislash:return 275;default:return 265;}}
inline Skill Kind(int sid){switch(sid){case 277:return Skill::Queen;case 266:return Skill::ATMG;case 257:return Skill::Spikes;case 263:return Skill::Afterimage;case 278:return Skill::DeathMark;case 256:return Skill::Gravity;case 276:return Skill::Overclock;case 275:return Skill::Omnislash;default:return Skill::Expose;}}
inline int Level(void* g,void* u,int sid){for(auto& p:Levels)if(p[0]==sid)return std::clamp(Curses::EvalCalc4(g,u,p[1],1,0),0,1000);return 0;}
inline bool NativePositions=false;
inline int Coordinate(void* unit,bool y){
 if(!unit)return 0;
 if(!NativePositions)return y?Curses::Y(unit):Curses::X(unit);
 // Runtime verification: +0x10/+0x12 in DynamicPath are the WALK TARGET.
 // Stationary sentries need not initialize them. Current positions are the
 // high words of the two fixed-point coordinates at +0 and +4.
 void* path=nullptr;std::memcpy(&path,static_cast<unsigned char*>(unit)+0x38,8);
 if(!path)return 0;
 unsigned short value=0;std::memcpy(&value,static_cast<unsigned char*>(path)+(y?6:2),2);return value;
}
inline Point Position(void* u){return {Coordinate(u,false)*2.0/3,Coordinate(u,true)*2.0/3};}
inline int Finish(void* game,void* attacker,void* target,int level){
 if(!Allocate||!Release||!DamageFor||!ResolveMelee||!SetTarget||!GetTarget)return 0;
 void* previous=GetTarget(game,attacker);SetTarget(attacker,target);
 Allocate(game,attacker,target,Finisher,level);
 int result=0;
 if(void* damage=DamageFor(attacker,target)){
  Release(game,attacker,Finisher,damage);
  result=ResolveMelee(game,attacker,target,6,1)&0xff;
 }
 SetTarget(attacker,previous);return result;
}
struct NativeHost:Host {
 void* owner(Id id){return Curses::Find(Game,0,id);}
 bool alive(Id id)override{return AuraFeatures::Alive(owner(id));}
 Point position(Id id)override{auto* u=owner(id);return u?Position(u):Point{};}
 bool find(Id o,Id id,Unit& out)override{
  auto* a=owner(o);auto* b=Curses::Find(Game,1,id);
  if(!a||!b||!Barbarian::SameAuraArea(a,b))return false;
  out={id,Position(b),Ramp::GetUnit(b,6,0),Ramp::GetUnit(b,7,0)};return true;
 }
 std::vector<Unit> targets(Id o,Point c,double radius)override{
  std::vector<Unit> out;auto* a=owner(o);if(!a)return out;
  AuraFeatures::Units(Game,1,[&](void* b){if(out.size()<256&&GuardianCombat::Hostile(Game,b)&&Barbarian::SameAuraArea(a,b)&&Distance(c,Position(b))<=radius)out.push_back({Barbarian::Id(b),Position(b),Ramp::GetUnit(b,6,0),Ramp::GetUnit(b,7,0)});});
  return out;
 }
 bool jump(Id o,Id t)override{
  Unit u;if(!alive(o)||!find(o,t,u)||u.hp<=0)return false;
  auto* a=owner(o);auto* b=Curses::Find(Game,1,t);
  if(!Curses::Cast(a,Blink,1,b,0))return false;
  return Distance(Position(a),Position(b))<=4;
 } // Teleport uses native collision handling; the explicit strike resolves melee range.
 bool clone(Id o,Id target,int level)override{
  auto* a=owner(o);auto* b=Curses::Find(Game,1,target);
  return a&&GuardianCombat::Hostile(Game,b)&&Curses::Cast(a,EchoSummon,level,b,0)!=0;
 }
 bool reserve(Id,int)override{return true;} // AuraBuffs owns the sole reservation list.
 void visual(Skill s,Id o,Point p,Tick duration)override{
  auto* a=owner(o);if(!a)return;
  if(s==Skill::Queen||s==Skill::Afterimage||s==Skill::ATMG)return; // Dedicated travelling blade / real pet below.
  const int spell=s==Skill::Spikes?SpikesVisualSkill:s==Skill::Gravity?GravityVisualSkill:s==Skill::DeathMark?DeathMarkVisualSkill:AfterimageVisualSkill;
  // A native cast replicates the client skill event, unlike a server-only
  // missile. Spikes are emitted at the moving owner; Gravity at its pet.
  void* origin=VisualOrigin?VisualOrigin:a;
  Curses::Cast(origin,spell,1,origin,0);
 }
 void pullSlow(Id o,Id t,Point center,int percent,Tick until)override{
  auto* a=owner(o);auto* b=Curses::Find(Game,1,t);if(!a||!GuardianCombat::Hostile(Game,b)||!Barbarian::SameAuraArea(a,b))return;
  Curses::Curse req{a,b,256,Level(Game,a,256),std::max(1,int(until-Ramp::Frame(Game))),67,-percent,GravitySlow,nullptr};
  Curses::OriginalApply(&req);
  // Battle Cry's verified native Taunt helper makes the victim approach
  // the caster rapidly. Cast from the trap, so the pull ends at the trap.
  if(VisualOrigin)Curses::Cast(VisualOrigin,Barbarian::PullSkill,std::max(1,Level(Game,a,256)),b,0);
 }
 Result hit(const Hit& h)override{
  auto* a=owner(h.owner);auto* b=Curses::Find(Game,1,h.target);
  if(!a||!GuardianCombat::Hostile(Game,b)||!Barbarian::SameAuraArea(a,b)||Depth)return {};
  const int before=Ramp::GetUnit(b,6,0);int result=0;
  ++Depth;ActiveHit=&h;
  ChargeReleaseDone=false;
  {
   // Native missile damage retains the attacker's equipment/proc ownership.
   ::Pulse::Params p{};p.flags=0x8021;p.owner=a;p.origin=b;p.target=b;p.missile=HitMissile;p.skill=HitSkill;p.level=std::max(1,Level(Game,a,Public(h.skill)));
   p.x=Coordinate(b,false)-1;p.y=Coordinate(b,true);p.targetX=p.x+2;p.targetY=p.y;p.range=1;
   if(void* m=::Pulse::OnCreate(Game,&p))result=Curses::NativeMissileHit(Game,m,b,0);
  }
  if(h.skill==Skill::Afterimage)Curses::Cast(a,EchoSummon,std::max(1,Level(Game,a,263)),b,0);
  if(h.skill==Skill::Queen||h.skill==Skill::ATMG){
   // The visual projectile is cast by the sentry. Mirror its player's two
   // native pierce sources so the blade's collision uses the same chance.
   if(h.skill==Skill::ATMG&&VisualOrigin)
    for(int stat:{156,166})Ramp::SetUnit(VisualOrigin,stat,std::clamp(Ramp::GetUnit(a,stat,0),0,100),0);
   Curses::Cast(VisualOrigin?VisualOrigin:a,h.skill==Skill::ATMG?ATMGVisualSkill:QueenVisualSkill,1,b,0);
  }
  if(Ramp::Log&&HitLogs++<40){char line[180]{};std::snprintf(line,sizeof(line),"Assassin attack: skill=%d owner=%u target=%u result=%d before=%d after=%d",Public(h.skill),h.owner,h.target,result,before,Ramp::GetUnit(b,6,0));Ramp::Log(line);}
  ActiveHit=nullptr;--Depth;
  auto* current=Curses::Find(Game,1,h.target);
  const int after=current?std::max(0,Ramp::GetUnit(current,6,0)):0;
  return {result!=0,std::max(0,before-after)};
 }
};
inline NativeHost HostAdapter;
inline Engine Scheduler(HostAdapter);
inline void Ensure(void* game){unsigned now=Ramp::Frame(game);if(Game!=game||now<LastFrame){Game=game;Depth=0;ActiveHit=nullptr;Areas={};Pets={};HitLogs=0;VisualOrigin=nullptr;Scheduler.reset(reinterpret_cast<std::uintptr_t>(game));}LastFrame=now;}
inline bool BuffPulse(void* game,void* unit,unsigned index,int level){
 const int sid=AuraBuffs::Definitions[index].skill;if(sid!=263&&sid!=278&&sid!=276&&sid!=257&&sid!=277)return false;
 Ensure(game);int state=AuraBuffs::Definitions[index].state;auto* list=Ramp::OriginalList(unit,state);
 if(!list){Curses::Curse r{unit,unit,sid,level,75,-1,0,state,nullptr};list=Curses::OriginalApply(&r);}
 if(!list)return false;
 const unsigned end=Ramp::Frame(game)+75;Ramp::SetExpiry(list,int(end));Ramp::Schedule(game,unit,12,int(end),0,0,0);
 Scheduler.syncBuff(Barbarian::Id(unit),Kind(sid),level,Ramp::Frame(game));return true;
}
inline void Disable(void* game,void* unit,unsigned index){const int s=AuraBuffs::Definitions[index].skill;if(s==263||s==278||s==276||s==257||s==277){Ensure(game);Scheduler.syncBuff(Barbarian::Id(unit),Kind(s),0,Ramp::Frame(game));}}
inline int Cast(void* game,void* unit,int skill,int level){if(skill!=275)return -1;if(!Curses::Player(unit))return 0;Ensure(game);AuraBuffs::Track(game,unit);return Scheduler.omnislash(Barbarian::Id(unit),level,Ramp::Frame(game))?1:0;}
inline void Projectile(void* game,void* raw,void* missile)noexcept{
 if(!raw||!missile)return;const auto& p=*static_cast<::Pulse::Params*>(raw);
 if(p.missile!=HitMissile||!ActiveHit)return;
 const auto& h=*ActiveHit;
 // Read the exact same hard-point formula used by the character sheet.
 const int percent=h.skill==Skill::Omnislash&&p.owner
  ? Curses::EvalCalc4(game,p.owner,275,p.level,h.weaponPercent):h.weaponPercent;
 for(int stat:{21,22,48,49,50,51,52,53,54,55,57,58,316,317}){
  std::int64_t v=h.flatPhysical?((stat==21||stat==22)?h.flatPhysical:0):std::int64_t{Ramp::GetUnit(missile,stat,0)}*percent/100;
  Ramp::SetUnit(missile,stat,int(std::clamp<std::int64_t>(v,0,0x3fffffff)),0);
 }
 // To-hit uses the owner snapshot and the skill's independent AR multiplier.
 if(p.owner){int ar=Ramp::GetUnit(p.owner,19,0);Ramp::SetUnit(missile,19,int(std::min<std::int64_t>(0x3fffffff,std::int64_t{ar}*(100+h.attackRating)/100)),0);}
}
inline void HitEvent(void* game,void* attacker,void* defender,void* packet){
 if(!packet||!Curses::Player(attacker)||!GuardianCombat::Hostile(game,defender))return;
 unsigned short flags=0;std::memcpy(&flags,static_cast<unsigned char*>(packet)+4,2);
 if(!(flags&1)||(flags&0x83b0))return;
 Ensure(game);
 // The primary clone hit releases the owner's charged attacks once. Splash
 // victims must not consume/release a second set of finishing-move charges.
 if(ActiveHit&&ActiveHit->skill==Skill::Omnislash&&ActiveHit->primary&&!ChargeReleaseDone&&Release&&SetTarget&&GetTarget){
  ChargeReleaseDone=true;void* previous=GetTarget(game,attacker);SetTarget(attacker,defender);
  Release(game,attacker,Finisher,packet);SetTarget(attacker,previous);
 }
 const int level=Level(game,attacker,265),hp=Ramp::GetUnit(defender,6,0),maxhp=Ramp::GetUnit(defender,7,0);
 // D2MOO SUnitDmg.h documents CRITICALSTRIKE=0x2000. Do not double an existing crit.
 if(level>0&&hp>0&&((hp>=maxhp)||(flags&0x2000))&&(!ActiveHit||ActiveHit->skill!=Skill::DeathMark)){
  int physical=Barbarian::ReadDamage(packet,0x18),total=Barbarian::ReadDamage(packet,0x134);
  const int base=(flags&0x2000)?physical/2:physical;
  const int wanted=int(std::min<std::int64_t>(0x3fffffff,std::int64_t{base}*(210+level)/100));
  const int delta=std::max(0,wanted-physical);physical+=delta;total=int(std::min<std::int64_t>(0x3fffffff,std::int64_t{total}+delta));flags|=0x2000;
  std::memcpy(static_cast<unsigned char*>(packet)+0x18,&physical,4);std::memcpy(static_cast<unsigned char*>(packet)+0x134,&total,4);std::memcpy(static_cast<unsigned char*>(packet)+4,&flags,2);
 }
 if(!Depth&&!Curses::ProcDepth){
  const int actual=std::clamp(Barbarian::ReadDamage(packet,0x134),0,std::max(0,hp));
  Scheduler.onHit(Barbarian::Id(attacker),Barbarian::Id(defender),100,actual,Ramp::Frame(game));
 }
}
inline void Retire(void* game,Pet& slot){
 auto* pet=Curses::Find(game,1,slot.id);auto* owner=Curses::Find(game,0,slot.owner);
 if(pet&&static_cast<unsigned*>(pet)[1]==slot.cls&&Barbarian::RemovePet&&Barbarian::RoomDelete&&Barbarian::DeleteUnit){
  if(void* room=SoulHarvest::Room(pet))Barbarian::RoomDelete(room,1,slot.id);
  if(owner)Barbarian::RemovePet(game,owner,slot.id,0);
  Barbarian::DeleteUnit(game,pet);
 }
 slot={};
}
inline bool NativeSentry(unsigned cls){for(unsigned candidate:NativeSentryClasses)if(cls==candidate)return true;return false;}
inline void PetsFrame(void* game){
 const unsigned now=Ramp::Frame(game);
 // Discovery never mutates the world's linked unit lists.
 AuraFeatures::Units(game,1,[&](void* pet){
  unsigned cls=static_cast<unsigned*>(pet)[1];
  if(cls!=ATMGClass&&cls!=GravityClass&&cls!=EchoClass&&!NativeSentry(cls))return;
  void* owner=Curses::SkeletonOwner(game,pet);if(!Curses::Player(owner))return;
  Pet* slot=nullptr;for(auto& p:Pets)if(p.id==Barbarian::Id(pet)&&p.cls==cls){slot=&p;break;}
  if(!slot)for(auto& p:Pets)if(p.id==Invalid){slot=&p;p={Barbarian::Id(pet),Barbarian::Id(owner),cls,now,now,now,0,80,1};
   p.level=std::max(1,Level(game,owner,cls==ATMGClass?266:cls==GravityClass?256:263));
   if(cls==EchoClass){
    // Visual-only Shadow Warrior: the owner-attributed echo already resolved its
    // one hit. Disable its own damage so a native animation cannot double it.
    for(int stat:{21,22,23,24,48,49,50,51,52,53,54,55,57,58})Ramp::SetUnit(pet,stat,0,0);
    if(Barbarian::ModeInfo&&Barbarian::ModeChange){
     auto enemies=HostAdapter.targets(p.owner,Position(pet),5);
     if(!enemies.empty())if(void* target=Curses::Find(game,1,enemies.front().id)){
      alignas(8) unsigned char info[0x80]{};Barbarian::ModeInfo(pet,4,info);std::memcpy(info+0x10,&target,8);Barbarian::ModeChange(game,info,0);
     }
    }
   }
   break;}
 });
 for(auto& t:Pets){
  if(t.id==Invalid)continue;
  auto* pet=Curses::Find(game,1,t.id);auto* owner=Curses::Find(game,0,t.owner);
  if(!pet||static_cast<unsigned*>(pet)[1]!=t.cls){t={};continue;}
  bool custom=t.cls==ATMGClass||t.cls==GravityClass||t.cls==EchoClass;
  if(!AuraFeatures::Alive(pet)||!AuraFeatures::Alive(owner)||!Barbarian::SameAuraArea(owner,pet)){
   if(custom)Retire(game,t);else t={};continue;
  }
  if(t.cls==EchoClass){if(now>=t.born+20)Retire(game,t);continue;}
  int boost=AuraFeatures::ActiveLevel(game,owner,276);int speed=boost?25+2*boost:0;
  if(!custom){
   if(now<t.next)continue;t.next=now+5;
   void* list=Ramp::OriginalList(pet,SpeedState);
   if(speed){
    if(!list){Curses::Curse request{owner,pet,276,boost,15,-1,0,SpeedState,nullptr};list=Curses::OriginalApply(&request);}
    if(list){Curses::Write(game,list,68,speed);Curses::Write(game,list,105,speed);Ramp::SetExpiry(list,now+15);Ramp::Schedule(game,pet,12,now+15,0,0,0);}
    if(SoulHarvest::Refresh)SoulHarvest::Refresh(pet);
    if(Curses::Update)Curses::Update(pet,"Overclock sentry animation rate",0);
   }else if(list)AuraBuffs::Expire(game,pet,SpeedState);
   continue;
  }
  if(now>=t.born+(t.cls==GravityClass?200U:750U)||t.shots<=0){Retire(game,t);continue;}
  const Point center=Position(pet);
  if(t.cls==GravityClass){
   if(now<t.next)continue;t.next=now+75;
   VisualOrigin=pet;HostAdapter.visual(Skill::Gravity,t.owner,center,15);VisualOrigin=nullptr;
   VisualOrigin=pet;
   for(auto& target:HostAdapter.targets(t.owner,center,6))HostAdapter.pullSlow(t.owner,target.id,center,std::min(60,20+t.level),now+75);
   VisualOrigin=nullptr;
  }else{
   t.credit+=std::min(now-t.last,5U)*8*(100+speed);t.last=now;
   auto enemies=HostAdapter.targets(t.owner,center,8);
   if(enemies.empty()){t.credit=std::min(t.credit,2499U);continue;}
   unsigned emitted=0;
   while(t.credit>=2500&&t.shots>0&&emitted++<8){
    t.credit-=2500;--t.shots;
    VisualOrigin=pet;
    HostAdapter.hit({Skill::ATMG,t.owner,enemies.front().id,Weapon(Skill::ATMG,t.level),Accuracy(Skill::ATMG,t.level)});
    VisualOrigin=nullptr;
   }
  }
 }
}
inline Ramp::ScheduleFn OriginalSchedule=nullptr;
inline int HastedDeadline(unsigned now,int deadline,int speed){
 if(deadline<=int(now)||speed<=0)return deadline;
 return int(now)+std::max(1,int((std::int64_t{deadline}-now)*100/(100+speed)));
}
inline void __fastcall Schedule(void* game,void* unit,int event,int deadline,int a,int b,int c){
 // Shorten only native Sentry AI delays. Never touch missile, expiry, regen,
 // player, hostile-monster or unrelated minion timers, and never add events.
 if(event==2&&Curses::Monster(unit)&&NativeSentry(static_cast<unsigned*>(unit)[1])){
  void* owner=Curses::SkeletonOwner(game,unit);
  if(Curses::Player(owner)){
   int level=AuraFeatures::ActiveLevel(game,owner,276);
   if(level>0)deadline=HastedDeadline(Ramp::Frame(game),deadline,25+2*level);
  }
 }
 OriginalSchedule(game,unit,event,deadline,a,b,c);
}
inline bool Install(const D2RL::PluginContext* context){
 struct Entry {std::uint64_t rva;const char* bytes;unsigned count;};
 const Entry checks[]={
  {0x572bd0,"\x40\x55\x53\x56\x57\x41\x56\x41\x57",9},
  {0x56c3e0,"\x4d\x85\xc9\x0f\x84\xea\x05\x00\x00",9},
  {0x44d660,"\x48\x89\x5c\x24\x08\x48\x89\x74\x24\x10",10},
  {0x44b2b0,"\x40\x53\x55\x56\x57\x41\x54\x41\x56",9},
  {0x34e0c0,"\x48\x89\x5c\x24\x10\x57\x48\x83\xec\x20",10},
  {0x48fe20,"\x40\x53\x48\x83\xec\x20\x48\x8b\xda",9}};
 for(auto& e:checks)if(!context->api->checkExpectedBytes(context,e.rva,reinterpret_cast<const unsigned char*>(e.bytes),e.count))return false;
 NativePositions=true;
 Allocate=reinterpret_cast<AllocateFn>(context->exeBase+0x572bd0);
 Release=reinterpret_cast<ReleaseFn>(context->exeBase+0x56c3e0);
 DamageFor=reinterpret_cast<Curses::DamageForFn>(context->exeBase+0x44d660);
 ResolveMelee=reinterpret_cast<Curses::MeleeFn>(context->exeBase+0x44b2b0);
 auto scheduling=D2RL::MakeInlineHook(CurseNative::ScheduleRva,CurseNative::ScheduleBytes,sizeof(CurseNative::ScheduleBytes),reinterpret_cast<void*>(Schedule),reinterpret_cast<void**>(&OriginalSchedule));
 if(!context->api->checkExpectedBytes(context,CurseNative::ScheduleRva,CurseNative::ScheduleBytes,sizeof(CurseNative::ScheduleBytes))||!context->api->installInlineHook(context,&scheduling))return false;
 SetTarget=reinterpret_cast<SetTargetFn>(context->exeBase+0x34e0c0);
 GetTarget=reinterpret_cast<GetTargetFn>(context->exeBase+0x48fe20);
 return true;
}
inline void Frame(void* game){
 Ensure(game);
 // Refresh learned levels and reconcile with the one authoritative reservation state.
 for(auto& p:AuraBuffs::Players)if(p.id!=Invalid)if(void* u=Curses::Find(game,0,p.id)){
  const int area=Barbarian::AuraArea?Barbarian::AuraArea(u):0;
  auto* location=&Areas[0];for(auto& a:Areas)if(a.first==p.id||a.first==0){location=&a;break;}
  if(location->first==p.id&&location->second!=area)Scheduler.forget(p.id);
  *location={p.id,area};
  Scheduler.setExpose(p.id,Level(game,u,265));
  for(int sid:{263,278,276,257,277}){const int i=AuraBuffs::Index(sid);if(i>=0)Scheduler.syncBuff(p.id,Kind(sid),p.costs[i]?p.levels[i]:0,Ramp::Frame(game));}
 }
 Scheduler.tick(reinterpret_cast<std::uintptr_t>(game),Ramp::Frame(game));
 PetsFrame(game);
}
} // namespace Assassin
