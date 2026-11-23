#pragma once
// Sorceress uses existing guarded frame, damage, projectile and skill-event hooks.
// Persistent records contain unit IDs only; no native pointer survives a frame.
#include "../leagues/craft_bindings.h"
namespace Sorceress {
#include "sorceress_bindings.h"
struct EnchantAttack {unsigned id=0xffffffffU,frame=0;};
inline std::array<EnchantAttack,512> EnchantAttacks{};
inline void* Session=nullptr;
inline unsigned Last=0,Next=0,Depth=0,Random=0x419e73U;
inline AuraBuffs::SkillFn NativeMorph=nullptr,NativeStorm=nullptr;
inline unsigned StormLogs=0,VisualLogs=0,EruptionLogs=0,AttackEvents=0;
inline int (*TestLevel)(void*,void*,int)=nullptr;
struct Zone {unsigned owner=0xffffffffU,start=0,next=0,until=0;int x=0,y=0,level=1,area=0;unsigned visualNext=0,seed=0;};
inline std::array<Zone,32> Zones{};
inline std::array<Zone,64> Eruptions{};
struct EruptionCombo {unsigned owner=0xffffffffU,until=0;int stacks=0;};
inline std::array<EruptionCombo,64> EruptionCombos{};
inline int EruptionPower=1;
struct FrostNovaTimer {unsigned pet=0xffffffffU,owner=0xffffffffU,next=0;};
inline std::array<FrostNovaTimer,512> FrostNovaTimers{};
inline bool ClaimFrostNova(unsigned pet,unsigned owner,unsigned now){
 FrostNovaTimer* slot=nullptr;
 for(auto& timer:FrostNovaTimers)if(timer.pet==pet&&timer.owner==owner){slot=&timer;break;}
 if(slot&&now<slot->next)return false;
 if(!slot)for(auto& timer:FrostNovaTimers)if(timer.pet==0xffffffffU||now>=timer.next){slot=&timer;break;}
 if(!slot)return false; // Never evict an active cooldown to permit an extra nova.
 *slot={pet,owner,now+25};return true;
}
inline int ComboPower(unsigned owner,unsigned now){
 for(const auto& c:EruptionCombos)if(c.owner==owner&&now<c.until)return std::clamp(c.stacks,1,3);
 return 1;
}
inline void AddComboHit(unsigned owner,unsigned now){
 EruptionCombo* slot=nullptr;
 for(auto& c:EruptionCombos)if(c.owner==owner){slot=&c;break;}
 if(!slot)for(auto& c:EruptionCombos)if(c.owner==0xffffffffU||now>=c.until){slot=&c;break;}
 if(slot){const int previous=slot->owner==owner&&now<slot->until?slot->stacks:0;*slot={owner,now+75,std::min(3,previous+1)};}
}
inline bool EruptionDispatch=false,EruptionProcWindow=false;
inline void Reset(void* game){const unsigned now=Ramp::Frame(game);if(Session!=game||now<Last){EnchantAttacks={};Zones={};Eruptions={};EruptionCombos={};FrostNovaTimers={};EruptionPower=1;EruptionDispatch=false;EruptionProcWindow=false;Next=0;Depth=0;Session=game;}Last=now;}
inline int Level(void* game,void* unit,int which){
 if(TestLevel)return TestLevel(game,unit,which);
 void* record=Curses::Record(Ramp::Version(game),LevelHelper);if(!record)return 0;
 int formula=0;std::memcpy(&formula,static_cast<unsigned char*>(record)+0x190+4*(which-1),4);
 return std::clamp(Curses::Evaluate(Ramp::Version(game),unit,formula,LevelHelper,1),0,1000);
}
inline int FormState(int skill){return skill==36?AvatarState:skill==39?QueenState:skill==43?MasterState:-1;}
inline void Refresh(void* unit){if(SoulHarvest::Refresh)SoulHarvest::Refresh(unit);if(Curses::Update)Curses::Update(unit,"Sorceress forms",0);}
inline void Appearance(void* unit,int state,bool active){
 if(GuardianCombat::ToggleState)GuardianCombat::ToggleState(unit,state,active?1:0);
 if(SoulHarvest::MarkGfxDirty)SoulHarvest::MarkGfxDirty(unit,state,1);
 Refresh(unit);
}
inline int AddedDamage(void* game,void* unit,int skill,int level,bool maximum){
 // Exactly like Enchant: evaluate enma/exma in whole points, including the
 // native skill/mastery calculation, before storing them on the player list.
 void* row=Curses::Record(Ramp::Version(game),skill);if(!row)return 0;
 int formula=0;std::memcpy(&formula,static_cast<unsigned char*>(row)+0x190+(maximum?4:0),4);
 return std::max(0,Curses::Evaluate(Ramp::Version(game),unit,formula,skill,level));
}
inline bool Pulse(void* game,void* unit,unsigned index,int level){
 const int skill=AuraBuffs::Definitions[index].skill,state=FormState(skill);if(state<0)return false;
 const unsigned now=Ramp::Frame(game),duration=std::max(75U,AuraBuffs::Next(game,unit,index,level)-now+25);
 void* list=Ramp::OriginalList(unit,state);const bool fresh=!list;
 if(!list){
  // Native transformation installs the skill-mode reset/removal callback.
  if(skill==36&&NativeMorph){NativeMorph(game,unit,skill,level);list=Ramp::OriginalList(unit,state);}
  else{Curses::Curse request{unit,unit,skill,level,static_cast<int>(duration),-1,0,state,nullptr};list=Curses::OriginalApply(&request);}
 }
 if(!list)return false;
 Ramp::SetExpiry(list,static_cast<int>(now+duration));Ramp::Schedule(game,unit,12,static_cast<int>(now+duration),0,0,0);
 if(skill==36){
  for(int stat:{68,105,99,102,67})Curses::Write(game,list,stat,10+level);
  Curses::Write(game,list,119,100+10*level);Curses::Write(game,list,76,25+3*level);
  Curses::Write(game,list,48,AddedDamage(game,unit,skill,level,false));Curses::Write(game,list,49,AddedDamage(game,unit,skill,level,true));
 }else if(skill==39){
  Curses::Write(game,list,171,30+10*level);Curses::Write(game,list,54,AddedDamage(game,unit,skill,level,false));Curses::Write(game,list,55,AddedDamage(game,unit,skill,level,true));Curses::Write(game,list,56,50);
 }else{Curses::Write(game,list,105,10+2*level);Curses::Write(game,list,50,AddedDamage(game,unit,skill,level,false));Curses::Write(game,list,51,AddedDamage(game,unit,skill,level,true));}
 if(fresh&&Curses::Register){const short events[]={7,8,-1,-1},functions[]={33,33,0,0};Curses::Register(game,unit,skill,level,skill,1,state,unit,events,functions,4);}
 if(fresh)Appearance(unit,state,true);else Refresh(unit);
 return true;
}
inline void Disable(void* game,void* unit,unsigned index){
 const int skill=AuraBuffs::Definitions[index].skill,state=FormState(skill);
 if(state<0)return;
 if(skill==36&&NativeMorph&&Ramp::OriginalList(unit,state))NativeMorph(game,unit,skill,std::max(1,AuraBuffs::Level(game,unit,index)));
 Appearance(unit,state,false);
}
inline void Storm(void* game,void* source,void* target,int skill,int level){
 // Preserve the native Diablo-wall initializer and client emission path.
 // The raw-emitter shortcut omitted initialization required by its child visuals.
 const int proc=skill==36?FireProc:skill==39?ColdProc:LightningProc;
 int result=Curses::Cast(source,proc,level,target,0);
 if(!result&&skill==36&&NativeStorm)result=NativeStorm(game,source,proc,level);
 if(Ramp::Log&&StormLogs++<12){char message[160]{};std::snprintf(message,sizeof(message),"Sorceress storm dispatch: form=%d level=%d result=%d frame=%u",skill,level,result,Ramp::Frame(game));Ramp::Log(message);}

}
inline void EnchantAttackRecovery(void* game,void* source){
 if(!game||!source||Depth||Curses::ProcDepth||EruptionDispatch||!AuraFeatures::Alive(source))return;
 void* list=Ramp::OriginalList(source,16);if(!list||!Curses::ListLevel)return;
 const int level=Curses::ListLevel(list);if(level<1)return;
 Reset(game);const unsigned id=Barbarian::Id(source),now=Ramp::Frame(game);
 EnchantAttack* slot=nullptr;
 for(auto& hit:EnchantAttacks){if(hit.id==id&&hit.frame==now)return;if(!slot&&(hit.id==0xffffffffU||hit.frame!=now))slot=&hit;}
 if(!slot)return;*slot={id,now};
 const int gain=(5+std::min(level,1000))*256;
 for(int stat:{6,8}){const int value=Ramp::GetUnit(source,stat,0),cap=std::max(0,Ramp::GetUnit(source,stat+1,0));Ramp::SetUnit(source,stat,int(std::min<std::int64_t>(cap,std::int64_t{value}+gain)),0);}
 Refresh(source);
}
inline bool EnchantWeaponSkill(int skill){return std::find(std::begin(LeagueCraft::WeaponSkills),std::end(LeagueCraft::WeaponSkills),skill)!=std::end(LeagueCraft::WeaponSkills);}
inline bool Event(void* game,int event,void* source,void* target,int skill,int level){
 // Enchant's native cast handler does not register these aura events.
 // Recovery is owned by direct attack completion/projectile creation below.
 if(skill==52)return true;

 if(skill!=36&&skill!=39&&skill!=43)return false;
 if((event!=7&&event!=8)||Depth||Curses::ProcDepth||!source||!target||level<1||!AuraFeatures::Alive(source))return true;
 // Storm projectiles must never start another generation of storms.
 // Ordinary melee attacks remain eligible; tornadoes get one explicit roll
 // per form per pulse, not one roll for every enemy in the area.
 if((event==8&&!EruptionProcWindow)||(EruptionDispatch&&!EruptionProcWindow))return true;
 ++AttackEvents;
 const int state=FormState(skill);
 if(!Ramp::OriginalList(source,state))return true;
 level=AuraBuffs::Level(game,source,AuraBuffs::Index(skill));
 if(level<1)return true;
 Random=Random*1664525u+1013904223u;
 if((Random>>8)%100>=25)return true;
 ++Depth;++Curses::ProcDepth;
 Storm(game,source,target,skill,std::min(level,1000));
 --Curses::ProcDepth;--Depth;return true;
}
// Explicitly replicate stationary server-created visual missiles to clients.
inline void SyncVisual(void* game,void* missile){
 if(!missile)return;
 SoulHarvest::CachePlayerIds(game);
 for(unsigned i=0;i<SoulHarvest::PlayerCount;++i)
  if(void* player=Curses::Find(game,0,SoulHarvest::PlayerIds[i]))SoulHarvest::SyncOrbToPlayer(game,missile,player);
}
inline bool StartEruption(void* game,void* owner,void* target,int level){
 Reset(game);const unsigned now=Ramp::Frame(game);
 AddComboHit(Barbarian::Id(owner),now);
 Zone* slot=nullptr;bool active=false;
 for(auto& z:Eruptions)if(z.owner==Barbarian::Id(owner)&&now<=z.until+5){slot=&z;active=true;break;}
 if(!slot)for(auto& z:Eruptions)if(z.owner==0xffffffffU||now>z.until+5){slot=&z;break;}
 if(slot){auto& z=*slot;
  // Recast at the newest target without stacking damage zones or advancing
  // the next hit deadline. Frequent attacks must not suppress pulses either.
  const unsigned next=active?z.next:now+25;
  z={};z.owner=Barbarian::Id(owner);z.start=now;z.next=next;z.until=now+75;z.level=std::clamp(level,1,1000);z.x=Curses::X(target);z.y=Curses::Y(target);z.area=Barbarian::AuraArea?Barbarian::AuraArea(owner):0;
  // Native skill dispatch creates the corresponding client animation, unlike
  // directly synchronizing a stationary server missile.
  ++Depth;++Curses::ProcDepth;
  const int visual=Curses::Cast(owner,EruptionVisualSkill,z.level,target,0);
  --Curses::ProcDepth;--Depth;
  if(Ramp::Log&&EruptionLogs++<12){char text[128]{};std::snprintf(text,sizeof(text),"Eruption start: level=%d visual=%d center=%d,%d",z.level,visual,z.x,z.y);Ramp::Log(text);}return true;
 }return false;
}
inline void EruptionTick(void* game,unsigned now){
 for(auto& c:EruptionCombos)if(c.owner!=0xffffffffU){
  if(now>=c.until||!AuraFeatures::Alive(Curses::Find(game,0,c.owner)))c={};
 }
 for(auto& z:Eruptions){
  if(z.owner==0xffffffffU||now>z.until+5)continue;
  void* owner=Curses::Find(game,0,z.owner);
  if(!AuraFeatures::Alive(owner)||(z.area&&Barbarian::AuraArea&&Barbarian::AuraArea(owner)!=z.area)){z.owner=0xffffffffU;continue;}
  if(now>=z.next&&z.next<=z.until+5){z.next=now+25;
   const int power=ComboPower(z.owner,now);
   // Snapshot IDs before native hit callbacks can change the unit lists.
   std::array<unsigned,256> ids{};unsigned count=0;
   AuraFeatures::Units(game,1,[&](void* target){
    auto dx=std::int64_t{Curses::X(target)}-z.x,dy=std::int64_t{Curses::Y(target)}-z.y;
    if(count<ids.size()&&GuardianCombat::Hostile(game,target)&&Barbarian::SameAuraArea(owner,target)&&4*(dx*dx+dy*dy)<=9*power*power)ids[count++]=Barbarian::Id(target);
   });
   EruptionDispatch=true;EruptionPower=power;unsigned procTarget=0xffffffffU;
   for(unsigned i=0;i<count;++i)if(void* target=Curses::Find(game,1,ids[i])){
    if(!GuardianCombat::Hostile(game,target))continue;
    Pulse::Params hit{};hit.flags=0x8021;hit.owner=owner;hit.origin=target;hit.target=target;
    hit.missile=EruptionHit;hit.skill=49;hit.level=z.level;hit.x=Curses::X(target)-1;hit.y=Curses::Y(target);hit.targetX=hit.x+2;hit.targetY=hit.y;hit.range=1;
    if(void* m=Pulse::OnCreate(game,&hit)){
     const int result=Curses::NativeMissileHit(game,m,target,0);
     if(result&&procTarget==0xffffffffU)procTarget=ids[i];
    }
   }
   EruptionProcWindow=true;
   if(procTarget!=0xffffffffU)if(void* target=Curses::Find(game,1,procTarget))
    for(int form:{36,39,43})Event(game,8,owner,target,form,1);
   EruptionProcWindow=false;EruptionDispatch=false;EruptionPower=1;
  }
 }
}
inline void Projectile(void* game,void* raw,void* missile) noexcept {
 if(!raw)return;const auto& p=*static_cast<Pulse::Params*>(raw);
 if(missile&&p.owner&&p.origin==p.owner&&EnchantWeaponSkill(p.skill)&&std::find(std::begin(LeagueCraft::WeaponMissiles),std::end(LeagueCraft::WeaponMissiles),p.missile)!=std::end(LeagueCraft::WeaponMissiles))EnchantAttackRecovery(game,p.owner);
 if(p.missile==EruptionHit&&missile){
  // Native snapshot retains weapon proc metadata; scale only damage fields.
  for(int stat:{21,22,48,49,50,51,52,53,54,55,57,58,316,317}){
   auto value=std::int64_t{Ramp::GetUnit(missile,stat,0)}/10;
   if(stat==48||stat==49||stat==50||stat==51||stat==54||stat==55){
    const int mastery=(stat==48||stat==49)?329:(stat==50||stat==51)?330:331;
    const int flat=(stat==49||stat==51||stat==55?10:5)+5*(std::clamp(p.level,1,1000)-1);
    value+=std::int64_t{flat}*256*std::max(0,100+Ramp::GetUnit(p.owner,mastery,0))/100;
   }
   Ramp::SetUnit(missile,stat,int(std::clamp<std::int64_t>(value*EruptionPower,0,0x3fffffff)),0);
  }return;
 }
 if(p.missile!=ConflagrationMissile||!Curses::Player(p.owner))return;
 Reset(game);const unsigned id=static_cast<unsigned*>(p.owner)[2],now=Ramp::Frame(game);
 for(const auto& z:Zones)if(z.owner==id&&z.start==now)return; // Native nova emits many missiles.
 for(auto& z:Zones)if(z.owner==0xffffffffU||now>=z.until){z={};z.owner=id;z.start=now;z.next=now+25;z.until=now+75;z.x=Curses::X(p.owner);z.y=Curses::Y(p.owner);z.level=std::clamp(p.level,1,1000);z.visualNext=now+5;z.seed=id*1664525U+now+1013904223U;z.area=Barbarian::AuraArea?Barbarian::AuraArea(p.owner):0;return;}
}
inline void Hit(void* game,void* attacker,void* defender,void* damage){
 if(damage&&attacker&&!(Barbarian::ReadDamage(damage,0)&2)&&EnchantWeaponSkill(Barbarian::UsedSkill(attacker))&&GuardianCombat::Hostile(game,defender))EnchantAttackRecovery(game,attacker);
 if(!EruptionDispatch&&!Depth&&!Curses::ProcDepth&&damage&&Curses::Player(attacker)&&Barbarian::UsedSkill(attacker)==49&&GuardianCombat::Hostile(game,defender)){
  unsigned short flags=0;std::memcpy(&flags,static_cast<unsigned char*>(damage)+4,2);
  if((flags&1)&&!(flags&0x4020)&&!(Barbarian::ReadDamage(damage,0)&2))StartEruption(game,attacker,defender,std::max(1,Level(game,attacker,3)));
 }

 if(Depth||Curses::ProcDepth||!Curses::Monster(attacker)||static_cast<unsigned*>(attacker)[1]!=FrostClass||!damage)return;
 unsigned short flags=0;std::memcpy(&flags,static_cast<unsigned char*>(damage)+4,2);
 if(!(flags&1)||(flags&0x22)||!GuardianCombat::Hostile(game,defender))return;
 void* owner=Curses::SkeletonOwner(game,attacker);if(!Curses::Player(owner))return;
 Reset(game);
 if(!ClaimFrostNova(Barbarian::Id(attacker),Barbarian::Id(owner),Ramp::Frame(game)))return;
 ++Depth;++Curses::ProcDepth;
 GuardianCombat::Cast(attacker,FrostNova,std::max(1,Level(game,owner,2)),defender);
 --Curses::ProcDepth;--Depth;
}
inline void RemoveRetiredFrostGuardians(void* game){
 if(!Barbarian::RoomDelete||!Barbarian::DeleteUnit||!Barbarian::RemovePet||!Curses::Find)return;
 // Snapshot IDs before deleting: unlinking during the native unit-list walk
 // could invalidate its iterator. Never remove a live or unrelated summon.
 std::array<unsigned,512> retired{};unsigned count=0;
 AuraFeatures::Units(game,1,[&](void* pet){
  if(static_cast<unsigned*>(pet)[1]==FrostClass&&!AuraFeatures::Alive(pet)&&count<retired.size())retired[count++]=Barbarian::Id(pet);
 });
 for(unsigned i=0;i<count;++i){
  void* pet=Curses::Find(game,1,retired[i]);
  if(!pet||static_cast<unsigned*>(pet)[1]!=FrostClass||AuraFeatures::Alive(pet))continue;
  void* owner=Curses::SkeletonOwner(game,pet);
  ++Depth;
  if(void* room=SoulHarvest::Room(pet))Barbarian::RoomDelete(room,1,retired[i]);
  if(Curses::Player(owner))Barbarian::RemovePet(game,owner,retired[i],0);
  Barbarian::DeleteUnit(game,pet);--Depth;
  for(auto& timer:FrostNovaTimers)if(timer.pet==retired[i])timer={};
 }
}
inline void Tick(void* game){
 Reset(game);const unsigned now=Ramp::Frame(game);if(now<Next)return;Next=now+5;
 RemoveRetiredFrostGuardians(game);
 EruptionTick(game,now);
 for(auto& z:Zones){
  if(z.owner==0xffffffffU||now>z.until+5)continue;
  void* owner=Curses::Find(game,0,z.owner);if(!AuraFeatures::Alive(owner)||(z.area&&Barbarian::AuraArea&&Barbarian::AuraArea(owner)!=z.area)){z.until=0;continue;}
  // Polling has an independent five-frame phase. Do not discard the last tick
  // merely because the first poll after its deadline lands at frame 76..79.
  while(now>=z.next&&z.next<=z.until){z.next+=25;
  ++Depth;++Curses::ProcDepth;
  AuraFeatures::Units(game,1,[&](void* target){
   if(!GuardianCombat::Hostile(game,target))return;
   // Bound the radius and reject unrelated rooms/acts before applying damage.
   auto dx=std::int64_t{Curses::X(target)}-z.x,dy=std::int64_t{Curses::Y(target)}-z.y;if(dx*dx+dy*dy>24*24)return;
   if(!Barbarian::SameAuraArea(owner,target))return;
   if(void* m=Curses::CreateAt(game,owner,BurnMissile,BurnSkill,z.level,Curses::X(target)-1,Curses::Y(target),Curses::X(target)+1,Curses::Y(target),target,1))Curses::NativeMissileHit(game,m,target,0);
  });
  --Curses::ProcDepth;--Depth;
  }
 }
}
}
