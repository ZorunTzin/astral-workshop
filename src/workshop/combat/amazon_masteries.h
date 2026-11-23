#pragma once
// Stances own temporary stat lists only; saved character/item stats are never edited.
namespace AmazonMasteries {
#include "amazon_masteries_bindings.h"
#include "amazon_masteries_native.h"
using SkillFn=AuraBuffs::SkillFn;
using BodyFn=unsigned char(__fastcall*)(void*);
inline BodyFn Body=nullptr;
inline std::array<SkillFn,191> OriginalStart{},OriginalDo{};
struct Player {unsigned id=0xffffffffu;int kind=-1;unsigned next=0;};
inline std::array<Player,16> Players{};
inline void* Session=nullptr;inline unsigned Last=0;
inline void reset(void* game){unsigned now=Ramp::Frame(game);if(Session!=game||now<Last){Session=game;Players={};}Last=now;}
inline int index(int skill){for(int i=0;i<3;++i)if(Definitions[i].skill==skill)return i;return -1;}
inline Player* player(void* game,void* unit){reset(game);if(!Curses::Player(unit)||static_cast<unsigned*>(unit)[1]!=0)return nullptr;auto id=Barbarian::Id(unit);for(auto& p:Players)if(p.id==id)return &p;for(auto& p:Players)if(p.id==0xffffffffu){p.id=id;return &p;}return nullptr;}
inline int weapon(void* unit){
 if(!Body||!LeagueStorage::Inventory||!LeagueStorage::First||!LeagueStorage::Next||!LeagueStorage::Code)return -1;
 auto inv=LeagueStorage::Inventory(unit);if(!inv)return -1;unsigned n=0;
 for(auto item=LeagueStorage::First(inv);item&&n++<4096;item=LeagueStorage::Next(item)){
  if(static_cast<unsigned*>(item)[0]!=4||static_cast<unsigned*>(item)[3]!=1)continue;
  auto body=Body(item);if(body!=4&&body!=5)continue;
  auto code=LeagueStorage::Code(item);
  for(auto w:Weapons)if((w.code&0xffffffu)==(code&0xffffffu))return w.kind;
 }return -1;
}
inline bool attack(int skill){for(int id:Attacks)if(id==skill)return true;return false;}
inline bool allowed(void* game,void* unit,int skill){
 if(!Curses::Player(unit)||static_cast<unsigned*>(unit)[1]!=0||!attack(skill))return true;
 auto p=player(game,unit);if(!p||p->kind<0)return true;
 if(weapon(unit)!=p->kind)return false;
 // Shared spear/javelin melee skills remain usable with either matching weapon.
 if(p->kind==0){for(int id:{6,7,11,12,15,16,20,21,22,25,26,27,31,35,2,4})if(skill==id)return false;}
 return true;
}
inline void clear(void* game,void* unit,int kind){
 auto list=Ramp::OriginalList(unit,Definitions[kind].state);if(!list)return;
 for(auto b:Bonuses)if(b.kind==kind)Curses::Write(game,list,b.stat,0);
 AuraBuffs::Expire(game,unit,Definitions[kind].state);
 if(SoulHarvest::Refresh)SoulHarvest::Refresh(unit);
 Curses::Update(unit,"Amazon Mastery disabled",0);
}
inline void pulse(void* game,void* unit,int kind,int level){
 int state=Definitions[kind].state;auto list=Ramp::OriginalList(unit,state);
 if(!list){Curses::Curse request{unit,unit,Definitions[kind].skill,level,50,-1,0,state,nullptr};list=Curses::OriginalApply(&request);}
 if(!list)return;
 for(auto b:Bonuses)if(b.kind==kind)Curses::Write(game,list,b.stat,std::min(b.cap,(b.base+b.step*level)/b.divisor));
 Ramp::SetExpiry(list,Ramp::Frame(game)+50);Ramp::Schedule(game,unit,12,Ramp::Frame(game)+50,0,0,0);
 if(SoulHarvest::Refresh)SoulHarvest::Refresh(unit);
 Curses::Update(unit,"Amazon Mastery",0);
}
inline int cast(void* game,void* unit,int skill,int level){
 int kind=index(skill);if(kind<0)return -1;
 auto p=player(game,unit);if(!p||AuraBuffs::Dead(unit)||level<1)return 0;
 int old=p->kind;for(int i=0;i<3;++i)clear(game,unit,i);
 p->kind=old==kind?-1:kind;p->next=0;
 if(p->kind>=0&&weapon(unit)==kind)pulse(game,unit,kind,level);
 return 1;
}
inline void frame(void* game){
 if(!game)return;reset(game);unsigned now=Ramp::Frame(game);
 for(auto& p:Players){if(p.id==0xffffffffu||p.kind<0||now<p.next)continue;p.next=now+5;
  auto unit=Curses::Find(game,0,p.id);if(!unit){p={};continue;}
  int level=Curses::EvalCalc4(game,unit,Definitions[p.kind].helper,1,0);
  if(AuraBuffs::Dead(unit)||level<1){clear(game,unit,p.kind);p.kind=-1;continue;}
  if(weapon(unit)!=p.kind){clear(game,unit,p.kind);continue;}
  pulse(game,unit,p.kind,level);
 }
}
template<int I,bool Start> int __fastcall gate(void* g,void* u,int s,int l){if(!allowed(g,u,s))return 0;auto fn=Start?OriginalStart[I]:OriginalDo[I];return fn?fn(g,u,s,l):0;}
template<bool Start,size_t... I> constexpr auto gates(std::index_sequence<I...>){return std::array<SkillFn,sizeof...(I)>{&gate<int(I),Start>...};}
inline bool install(const D2RL::PluginContext* c){
 if(!c->api->checkExpectedBytes(c,0x36ca10,BodyGuard,sizeof(BodyGuard)))return false;
 Body=reinterpret_cast<BodyFn>(c->exeBase+0x36ca10);
 auto starts=reinterpret_cast<SkillFn*>(c->exeBase+0x238e710),dos=reinterpret_cast<SkillFn*>(c->exeBase+0x238ea00);
 for(auto b:StartBindings)if(starts[b.index]!=reinterpret_cast<SkillFn>(c->exeBase+b.rva))return false;
 for(auto b:DoBindings)if(dos[b.index]!=reinterpret_cast<SkillFn>(c->exeBase+b.rva))return false;
 constexpr auto sg=gates<true>(std::make_index_sequence<191>{}),dg=gates<false>(std::make_index_sequence<191>{});
 for(auto b:StartBindings){auto old=starts[b.index];OriginalStart[b.index]=old;if(!c->api->patchWrite(c,0x238e710+8*b.index,&old,8,reinterpret_cast<uint64_t>(sg[b.index]),8))return false;}
 for(auto b:DoBindings){auto old=dos[b.index];OriginalDo[b.index]=old;if(!c->api->patchWrite(c,0x238ea00+8*b.index,&old,8,reinterpret_cast<uint64_t>(dg[b.index]),8))return false;}
 c->LogInfo("Amazon Masteries: three exclusive stances; server start/do weapon gates enabled.");return true;
}
}
