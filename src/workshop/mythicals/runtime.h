#pragma once
#include "model.h"
#include "bindings.h"
namespace Mythicals {
#include "bonds.inc"
inline bool Power(void* unit,int skill){return Ramp::GetUnit(unit,MarkerStat,skill)>0;}
inline void Hit(void* game,void* attacker,void* defender,void* damage){
 if(!game||!damage||!Curses::Player(attacker)||!Curses::Monster(defender)||!GuardianCombat::Hostile(game,defender))return;
 auto* p=static_cast<unsigned char*>(damage);unsigned short flags=0;int total=0,physical=0;
 std::memcpy(&flags,p+4,2);std::memcpy(&total,p+0x134,4);std::memcpy(&physical,p+0x18,4);
 if(!(flags&1)||(flags&0x3b0)||total<=0)return;
 const bool prism=Power(attacker,Power0),voidforge=false;
 // These powers use resolved physical damage; resistance is then applied to
 // each added element. They do not spawn another hit or recursively trigger procs.
 if(physical>0&&(prism||voidforge)){
  unsigned element=voidforge?SetSelectors::Draw(3):3;
  const unsigned offsets[]={0x20,0x2c,0x24};const int resists[]={39,43,41};
  for(unsigned n=0;n<3;++n){int percent=(prism?25:0)+(element==n?100:0);if(!percent)continue;
   int current=0;std::memcpy(&current,p+offsets[n],4);
   int extra=MythicalModel::ElementExtra(physical,percent,Ramp::GetUnit(defender,resists[n],0),current,total);
   current+=extra;total+=extra;std::memcpy(p+offsets[n],&current,4);
  }
  std::memcpy(p+0x134,&total,4);
 }
 const bool highlord=Power(attacker,Power18),orion=physical>0&&Power(attacker,Power20),tsuku=physical>0&&Power(attacker,Power27);
 if(highlord||orion||tsuku){int multiplier=MythicalModel::Multiplier(highlord,orion,tsuku,SetSelectors::Draw(100));if(multiplier>1)AuraFeatures::ScaleResolvedDamage(damage,multiplier,1);}
}
// Layers 43/44 are effect tags, not item ordinals; multiple variants share them.
inline void Renewal(void* game,void* unit,unsigned now){
 const bool mage=Power(unit,44),screams=Power(unit,43);
 if((!mage&&!screams)||!AuraFeatures::Alive(unit)){if(Ramp::OriginalList(unit,RenewalState))AuraBuffs::Expire(game,unit,RenewalState);return;}
 void* list=Ramp::OriginalList(unit,RenewalState);
 if(!list&&Curses::OriginalApply){Curses::Curse request{unit,unit,0,1,15,-1,0,RenewalState,nullptr};list=Curses::OriginalApply(&request);}
 if(!list)return;
 // HP and mana are 8-bit fixed point. Recovery stats are fixed-point units/frame (25 fps).
 auto rate=[&](int stat,int percent){return static_cast<int>(std::min<std::int64_t>(0x3fffffff,std::int64_t{std::max(0,Ramp::GetUnit(unit,stat,0))}*percent/2500));};
 Curses::Write(game,list,74,rate(7,mage?10:5));
 Curses::Write(game,list,26,mage?rate(9,15):0);
 Curses::Write(game,list,27,screams?400:0);
 Ramp::SetExpiry(list,static_cast<int>(now+15));Ramp::Schedule(game,unit,12,static_cast<int>(now+15),0,0,0);
 if(SoulHarvest::Refresh)SoulHarvest::Refresh(unit);
}
inline void* Session=nullptr;inline unsigned Last=0,Next=0;
inline void Frame(void* game){
 if(!game)return;const unsigned now=Ramp::Frame(game);
 if(Session!=game||now<Last){Session=game;Next=0;}Last=now;if(now<Next)return;Next=now+5;
 SoulHarvest::CachePlayerIds(game);
 for(unsigned i=0;i<SoulHarvest::PlayerCount;++i){
  void* unit=Curses::Find(game,0,SoulHarvest::PlayerIds[i]);if(!unit)continue;
  Renewal(game,unit,now);
  const int powers[]={Power28,Power29,Power30,Power31,Power32,Power33,Power34};unsigned mask=0;
  for(unsigned j=0;j<7;++j)if(Power(unit,powers[j]))mask|=1u<<j;
  if(!MythicalModel::Bonded(mask)||!AuraFeatures::Alive(unit)){if(Ramp::OriginalList(unit,BondState))AuraBuffs::Expire(game,unit,BondState);continue;}
  void* list=Ramp::OriginalList(unit,BondState);
  if(!list&&Curses::OriginalApply){Curses::Curse request{unit,unit,0,1,15,-1,0,BondState,nullptr};list=Curses::OriginalApply(&request);}
  if(!list)continue;
  for(const auto& b:BondBonuses)Curses::Write(game,list,b[1],0);
  for(const auto& b:BondBonuses)if(Power(unit,b[0]))Curses::Write(game,list,b[1],Curses::Read(game,list,b[1])+b[2]);
  Ramp::SetExpiry(list,static_cast<int>(now+15));Ramp::Schedule(game,unit,12,static_cast<int>(now+15),0,0,0);
  if(SoulHarvest::Refresh)SoulHarvest::Refresh(unit);
  if(SoulHarvest::MarkGfxDirty)SoulHarvest::MarkGfxDirty(unit,BondState,1);
 }
}
}
