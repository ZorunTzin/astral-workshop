#pragma once
#include <atomic>
// New PoE powers operate on native hit packets and scoped combat state only.
namespace PoeMythicals {
#include "poe_bindings.h"
constexpr int Sin=267,Svalinn=304,Dream=309,Rathpith=310,Aegis=311,Nimis=312,Prototype=313;
constexpr unsigned Invalid=0xffffffffU;
inline int Stat(void* u,int s){return u?Ramp::GetUnit(u,s,0):0;}
inline bool Has(void* u,int s){return Curses::Player(u)&&Stat(u,s)>0;}
inline int Clamp(long long n){return int(std::clamp<long long>(n,0,0x3fffffff));}
inline int Read(void* p,unsigned o){int v=0;if(p)std::memcpy(&v,static_cast<unsigned char*>(p)+o,4);return v;}
inline void Write(void* p,unsigned o,int v){std::memcpy(static_cast<unsigned char*>(p)+o,&v,4);}
inline bool Near(void* a,void* b){if(!a||!b||!Barbarian::SameAuraArea(a,b))return false;long long x=Amazon::X(a)-Amazon::X(b),y=Amazon::Y(a)-Amazon::Y(b);return x*x+y*y<=81;}
inline int Defense(void* u){return Clamp((1LL*std::max(0,Stat(u,31))+std::max(0,Stat(u,2))/4)*std::max(0,100+Stat(u,171))/100);}
inline bool Spell(int skill){return std::find(std::begin(SpellSkills),std::end(SpellSkills),skill)!=std::end(SpellSkills);}
inline int PrototypeStat(void* u){int n=Stat(u,Prototype);return n==1?39:n==2?43:n==3?41:n==4?45:-1;}
inline int PrototypeOffset(void* u){int n=Stat(u,Prototype);return n==1?0x20:n==2?0x34:n==3?0x2c:n==4?0x38:-1;}
inline int Resistance(void* game,void* u,int stat){int penalty=game?std::clamp(int(static_cast<unsigned char*>(game)[0x104]),0,2):0;return std::clamp(Stat(u,stat)-(penalty==2?100:penalty==1?40:0),-100,std::min(95,75+Stat(u,stat+1)));}
inline thread_local int CastingSkill=-1;
struct Barrier{unsigned id=Invalid,until=0;int value=0;};
inline std::array<Barrier,16> Barriers{};
struct Flight{unsigned id=Invalid,cls=0,owner=Invalid,born=0,end=0;int skill=-1,level=1,x=0,y=0;bool returning=false,ready=false,published=false,extra=false;std::array<int,std::size(Amazon::DamageStats)> damage{};};
inline std::array<Flight,4096> Flights{};
inline std::vector<Flight> Pending,Extras;
inline thread_local bool ReturnSpawn=false,ExtraSpawn=false;
inline void* Session=nullptr;inline unsigned Last=0;
inline void Reset(void* game){unsigned n=Ramp::Frame(game);if(Session!=game||n<Last){Session=game;Flights={};Barriers={};Pending.clear();Extras.clear();}Last=n;}
inline Barrier* Ward(void* u){unsigned id=Barbarian::Id(u);for(auto& b:Barriers)if(b.id==id)return &b;for(auto& b:Barriers)if(b.id==Invalid){b.id=id;return &b;}return nullptr;}
inline void Blocked(void* game,void* u){Reset(game);if(Has(u,Aegis))if(auto b=Ward(u)){b->value=Clamp(1LL*Defense(u)*256*2/100);b->until=Ramp::Frame(game)+125;}}
using BlockFn=int(__fastcall*)(void*,void*,void*,int,int);
using BlockRateFn=int(__fastcall*)(void*,int);
using MovingFn=int(__fastcall*)(void*);
using DodgeFn=int(__fastcall*)(void*,void*,int);
inline BlockFn OriginalBlock=nullptr;inline BlockRateFn BlockRate=nullptr;inline MovingFn Moving=nullptr;inline DodgeFn Dodge=nullptr;
inline int __fastcall CappedBlockRate(void* u,int expansion){int rate=BlockRate(u,expansion);return Has(u,Svalinn)?std::min(65,rate):rate;}
inline int __fastcall Block(void* g,void* a,void* d,int avoid,int allow){
 int result=0;
 if(!allow||!Has(d,Svalinn))result=OriginalBlock(g,a,d,avoid,allow);
 else{int rate=std::clamp(CappedBlockRate(d,static_cast<unsigned char*>(g)[0x106]!=1),0,65);if(Moving(d)&&static_cast<unsigned*>(d)[3]!=2)rate/=3;
  result=(int(SetSelectors::Draw(100))<rate||int(SetSelectors::Draw(100))<rate)?1:Dodge(a,d,avoid);}
 if(result&1)Blocked(g,d);return result;
}
// Native D2R component descriptors contain a direct damage-field pointer.
struct DamageInfo {void* game;void* difficulty;void* attacker;void* defender;int attackerMonster,defenderMonster;void* damage;int reduction[3];};
struct Component {int* value;int resist,maxres,pierce,absorbPct,absorb;int unknown,reduction,monsterOnly,multiply,padding;const char* name;unsigned char tail[8];};
static_assert(offsetof(Component,resist)==8&&offsetof(Component,name)==48&&sizeof(Component)==64);
static_assert(offsetof(DamageInfo,damage)==40&&offsetof(DamageInfo,reduction)==48);
using ComponentFn=void(__fastcall*)(DamageInfo*,const Component*,int);
inline ComponentFn OriginalComponent=nullptr;
inline void __fastcall Resist(DamageInfo* info,const Component* component,int noAbsorb){
 if(!info||!component||!component->value){OriginalComponent(info,component,noAbsorb);return;}
 int desired=999;auto a=info->attacker,d=info->defender;const int stat=component->resist;
 if(Curses::Monster(d)&&Curses::Player(a)&&Near(a,d)){
  if(Has(a,Sin)&&stat==37)desired=0;
 }
 if(desired==999){OriginalComponent(info,component,noAbsorb);return;}
 Component copy=*component;DamageInfo local=*info;
 // Apply the native flat reduction once, then our resistance. Native absorbs
 // still execute; no unit stat or saved value is temporarily overwritten.
 int reduction=copy.reduction>=0&&copy.reduction<3?local.reduction[copy.reduction]:0;
 long long value=std::max(0,*copy.value-(noAbsorb?0:reduction));
 if(noAbsorb)desired=std::min(0,desired);
 *copy.value=Clamp(value*(100-desired)/100);copy.resist=-1;copy.maxres=-1;copy.pierce=-1;local.defenderMonster=1;
 for(int& n:local.reduction)n=0;OriginalComponent(&local,&copy,noAbsorb);
}
using ResolveFn=void(__fastcall*)(void*,void*,void*,void*);
inline ResolveFn OriginalResolve=nullptr;
inline bool SpellHit(){if(Spell(CastingSkill))return true;void* m=Amazon::CurrentHitMissile;if(!m)return false;auto& f=Flights[Barbarian::Id(m)%Flights.size()];return f.id==Barbarian::Id(m)&&Spell(f.skill);}
inline void BeforeResolve(void* g,void* a,void*,void* p){
 if(!Curses::Player(a)||!p)return;
 bool spell=SpellHit();int bonus=0;if(Has(a,Rathpith)&&spell)bonus+=std::max(0,Stat(a,7)/256)/100*3;
 if(bonus)for(unsigned off:{0x18u,0x20u,0x24u,0x2cu,0x30u,0x34u,0x38u})Write(p,off,Clamp(1LL*std::max(0,Read(p,off))*(100LL+bonus)/100));
 if(Has(a,Sin)){long long magic=std::max(0,Read(p,0x30));for(unsigned off:{0x20u,0x2cu,0x34u}){magic+=std::max(0,Read(p,off));Write(p,off,0);}Write(p,0x30,Clamp(magic));Write(p,0x118,0);Write(p,0x11c,0);}
}
inline void Absorb(void* g,void* d,void* p){
 if(!Curses::Player(d)||!p)return;Reset(g);auto b=Ward(d);if(!b)return;
 if(!Has(d,Aegis)||!AuraFeatures::Alive(d)||b->until<=Ramp::Frame(g)){b->value=0;return;}
 unsigned short flags=0;std::memcpy(&flags,static_cast<unsigned char*>(p)+4,2);if(!(flags&1)||(flags&0x83b0))return;
 int used=0;for(unsigned off:{0x18u,0x20u,0x2cu,0x30u,0x34u}){int old=std::max(0,Read(p,off)),take=std::min(old,b->value);Write(p,off,old-take);b->value-=take;used+=take;}
 Write(p,0x134,std::max(0,Read(p,0x134)-used));
}
inline void __fastcall Resolve(void* g,void* a,void* d,void* p){BeforeResolve(g,a,d,p);OriginalResolve(g,a,d,p);Absorb(g,d,p);}
using SkillFn=AuraBuffs::SkillFn;
inline std::array<SkillFn,172> NativeSkills{};
inline thread_local unsigned CastDepth=0;
inline void Sacrifice(void* u){int hp=Stat(u,6);if(hp>256){Ramp::SetUnit(u,6,std::max(256,hp-hp/10),0);if(Curses::Update)Curses::Update(u,__FILE__,__LINE__);}}
// Wrap server do-functions after existing Workshop handlers; only successful
// damaging spell casts pay once. Missile creation itself never pays life.
template<int I> int __fastcall Cast(void* g,void* u,int s,int l){int prior=CastingSkill;CastingSkill=s;bool outer=CastDepth++==0;int result=NativeSkills[I]?NativeSkills[I](g,u,s,l):0;--CastDepth;CastingSkill=prior;if(outer&&result&&Spell(s)&&Has(u,Rathpith))Sacrifice(u);return result;}
template<size_t... I> constexpr auto Casts(std::index_sequence<I...>){return std::array<SkillFn,sizeof...(I)>{&Cast<int(I)>...};}
inline bool Eligible(int cls){return cls!=Amazon::Return&&(cls==Amazon::MultiArrow||cls==Amazon::MultiBolt||(Amazon::Stage(cls)<0&&std::find(std::begin(ReturnClasses),std::end(ReturnClasses),cls)!=std::end(ReturnClasses)));}
inline int ReturnFrames(int cls,int level,int dx,int dy){auto it=std::find(std::begin(ReturnClasses),std::end(ReturnClasses),cls);if(it==std::end(ReturnClasses))return 1;auto& v=ReturnVelocity[it-std::begin(ReturnClasses)];double velocity=std::max(1,v[0]+v[1]*std::max(0,level-1))*0.75;return std::clamp(int(std::floor(std::hypot(double(dx),double(dy))*16/velocity)),1,125);}
inline bool Filter(void* g,void* raw)noexcept{
 auto p=static_cast<Pulse::Params*>(raw);if(!p||!ExtraSpawn||ReturnSpawn||!Has(p->owner,Nimis)||!Eligible(p->missile))return true;
 double angle=SetSelectors::Draw(36000)*0.00017453292519943295;p->flags=0x8021U;p->target=nullptr;p->origin=p->owner;p->x=Amazon::X(p->owner);p->y=Amazon::Y(p->owner);p->targetX=p->x+int(std::lround(30*std::cos(angle)));p->targetY=p->y+int(std::lround(30*std::sin(angle)));p->range=25;p->initializeFunction=0;p->initializeArgument=0;return true;
}
inline void Created(void* g,void* raw,void* m)noexcept{
 auto p=static_cast<Pulse::Params*>(raw);if(!p||!m||!Curses::Player(p->owner))return;Reset(g);unsigned id=Barbarian::Id(m);auto& f=Flights[id%Flights.size()];f={};f.id=id;f.cls=p->missile;f.owner=Barbarian::Id(p->owner);f.skill=p->skill;f.level=p->level;f.born=Ramp::Frame(g);f.end=f.born+250;f.extra=!ExtraSpawn&&!ReturnSpawn&&Has(p->owner,Nimis)&&Eligible(p->missile);f.returning=!ExtraSpawn||ReturnSpawn;if(!f.returning)Write(m,0x124,Read(m,0x124)|0x00200000u);
}
inline void BeforeTick(void* g,void* m)noexcept{
 if(!m)return;Reset(g);auto& f=Flights[Barbarian::Id(m)%Flights.size()];if(f.id!=Barbarian::Id(m)||(!f.extra&&f.returning))return;
 f.x=Amazon::X(m);f.y=Amazon::Y(m);for(unsigned i=0;i<f.damage.size();++i)f.damage[i]=Ramp::GetUnit(m,Amazon::DamageStats[i],0);f.ready=true;if(f.extra){f.extra=false;if(Extras.size()<4096)Extras.push_back(f);}
}
inline void FinishTick(void*,unsigned id,int result)noexcept{auto& f=Flights[id%Flights.size()];if(result==2&&f.id==id&&!f.returning&&f.ready){if(Pending.size()<4096)Pending.push_back(f);f.returning=true;}}
inline std::atomic<int> DisplayDefense{0},DisplayLife{0};
inline void Display(void* g,void* u){
 if(!Curses::Player(u))return;
 const bool alive=AuraFeatures::Alive(u);int dream=alive&&Has(u,Dream)?Defense(u)/100:0,rath=alive&&Has(u,Rathpith)?std::max(0,Stat(u,7)/256)/100*3:0;
 DisplayDefense.store(Defense(u),std::memory_order_relaxed);DisplayLife.store(std::max(0,Stat(u,7)/256),std::memory_order_relaxed);
 auto list=Ramp::OriginalList(u,DisplayState);
 if(!list&&(dream||rath)){Curses::Curse req{u,u,0,1,15,-1,0,DisplayState,nullptr};list=Curses::OriginalApply(&req);}
 if(!list)return;bool changed=WorkshopShrines::Set(g,list,25,dream);changed=WorkshopShrines::Set(g,list,314,rath)||changed;
 if(!dream&&!rath){AuraBuffs::Expire(g,u,DisplayState);if(changed){if(SoulHarvest::MarkGfxDirty)SoulHarvest::MarkGfxDirty(u,DisplayState,1);Gull::Refresh(u);}return;}
 Ramp::SetExpiry(list,int(Ramp::Frame(g)+15));Ramp::Schedule(g,u,12,int(Ramp::Frame(g)+15),0,0,0);
 if(changed){if(SoulHarvest::MarkGfxDirty)SoulHarvest::MarkGfxDirty(u,DisplayState,1);Gull::Refresh(u);}
}
// UTF-8 U+00FF is required by the public tooltip API, not a raw 0xff byte.
inline const D2RL::ItemService* Items=nullptr;
inline void __cdecl PrototypeTooltip(const D2RL::PluginContext* c,D2RL::SharedEvents::ItemTooltipEvent* e,void*) noexcept{
 if(!e||!e->text||!Items)return;D2RL::Items::ItemInfo info{.structSize=D2RL::Items::ItemInfoSize};
 if(Items->getItemInfo(c,e->item,&info)!=D2RL::Items::Result::Success)return;
 const char* element=nullptr;
 if(info.code==D2RL::Items::MakeItemCode("Y88"))element="fire";
 if(info.code==D2RL::Items::MakeItemCode("Y89"))element="cold";
 if(info.code==D2RL::Items::MakeItemCode("Y90"))element="lightning";
 if(info.code==D2RL::Items::MakeItemCode("Y91"))element="poison";
 if(!element)return;
 std::string text="\xee\x81\xbe" "4Nearby enemies have ";text+=element;text+=" resistance equal to yours";text+="\xee\x81\xbe" "0";
 if(text.size()>=e->capacity)return;std::memcpy(e->text,text.c_str(),text.size()+1);e->length=unsigned(text.size());
}
// A single timed aura list adjusts the actual monster resistance. Subtract
// our previous delta before recalculation so repeated pulses never compound.
inline void AuraTarget(void* g,void* enemy,const std::vector<void*>& owners){
 if(!Curses::Monster(enemy))return;auto list=Ramp::OriginalList(enemy,PrototypeState);
 std::array<int,4> desired{999,999,999,999};bool active=false;
 if(GuardianCombat::Hostile(g,enemy))for(auto u:owners)if(AuraFeatures::Alive(u)&&Has(u,Prototype)&&Near(u,enemy)){
  int n=Stat(u,Prototype)-1;if(n>=0&&n<4){desired[n]=std::min(desired[n],Resistance(g,u,PrototypeStat(u)));active=true;}}
 if(!list&&active){Curses::Curse req{enemy,enemy,0,1,10,-1,0,PrototypeState,nullptr};list=Curses::OriginalApply(&req);}if(!list)return;
 bool changed=false;const int stats[]={39,43,41,45};
 for(int i=0;i<4;++i){int old=Curses::Read(g,list,stats[i]);int base=Stat(enemy,stats[i])-old;int delta=desired[i]==999?0:desired[i]-base;changed=WorkshopShrines::Set(g,list,stats[i],delta)||changed;}
 if(active){Ramp::SetExpiry(list,int(Ramp::Frame(g)+10));Ramp::Schedule(g,enemy,12,int(Ramp::Frame(g)+10),0,0,0);}else AuraBuffs::Expire(g,enemy,PrototypeState);
 if(changed){if(SoulHarvest::MarkGfxDirty)SoulHarvest::MarkGfxDirty(enemy,PrototypeState,1);Gull::Refresh(enemy);}
}
inline void PrototypeAura(void* g){
 if(Ramp::Frame(g)%5)return;std::vector<void*> owners;
 AuraFeatures::Units(g,0,[&](void* u){if(Has(u,Prototype)&&AuraFeatures::Alive(u))owners.push_back(u);});
 AuraFeatures::Units(g,1,[&](void* u){AuraTarget(g,u,owners);});
}
inline void Frame(void* g){
 PrototypeAura(g);
 AuraFeatures::Units(g,0,[&](void* u){Display(g,u);});
 Reset(g);for(auto& b:Barriers)if(b.id!=Invalid){auto u=Curses::Find(g,0,b.id);if(!u||!Has(u,Aegis)||!AuraFeatures::Alive(u)||b.until<=Ramp::Frame(g))b={};}
 for(auto& f:Flights)if(f.id!=Invalid&&(!f.returning||f.extra)){auto m=Curses::Find(g,3,f.id);if(m&&static_cast<unsigned*>(m)[1]==f.cls){if(!f.returning&&!f.published&&Amazon::PublishMissiles){void* shots[]={m};Amazon::PublishMissiles(m,shots,1,m,nullptr,0);f.published=true;}BeforeTick(g,m);}else if(f.ready&&!f.returning){if(Pending.size()<4096)Pending.push_back(f);f.returning=true;}if(f.end<=Ramp::Frame(g))f.returning=true;}
 auto extras=std::move(Extras);Extras.clear();
 for(auto& f:extras){auto u=Curses::Find(g,0,f.owner);if(!AuraFeatures::Alive(u)||!Has(u,Nimis))continue;
  Pulse::Params p{};p.flags=0x8021;p.owner=u;p.origin=u;p.missile=f.cls==Amazon::MultiArrow?0:f.cls==Amazon::MultiBolt?31:int(f.cls);p.skill=f.skill;p.level=f.level;
  ExtraSpawn=true;auto m=Pulse::OnCreate(g,&p);ExtraSpawn=false;
  if(m)for(unsigned i=0;i<f.damage.size();++i)Ramp::SetUnit(m,Amazon::DamageStats[i],f.damage[i],0);
 }
 auto pending=std::move(Pending);Pending.clear();
 for(auto& f:pending){auto u=Curses::Find(g,0,f.owner);if(!AuraFeatures::Alive(u)||!Has(u,Nimis))continue;Pulse::Params p{};p.flags=0x8021;p.owner=u;p.origin=u;p.missile=int(f.cls);p.skill=f.skill;p.level=f.level;p.x=f.x;p.y=f.y;p.targetX=Amazon::X(u);p.targetY=Amazon::Y(u);double dx=p.targetX-p.x,dy=p.targetY-p.y,dist=std::hypot(dx,dy);if(dist<2)continue;p.x+=int(std::lround(dx/dist));p.y+=int(std::lround(dy/dist));p.range=ReturnFrames(f.cls,f.level,p.targetX-p.x,p.targetY-p.y);
  ReturnSpawn=true;auto m=Pulse::OnCreate(g,&p);ReturnSpawn=false;if(m){for(unsigned i=0;i<f.damage.size();++i)Ramp::SetUnit(m,Amazon::DamageStats[i],f.damage[i],0);unsigned flags=Read(m,0x124);Write(m,0x124,int(flags|0x00200000u));if(Amazon::PublishMissiles){void* shots[]={m};Amazon::PublishMissiles(m,shots,1,m,nullptr,0);}}}
}
inline MonsterRuntime::DropFn NativeDrop=nullptr;
inline std::uintptr_t __fastcall Drop(void* g,void* u,void* k,unsigned short tc,std::uintptr_t a5,std::uintptr_t a6,std::uintptr_t a7,std::uintptr_t a8,std::uintptr_t a9,std::uintptr_t a10){auto r=NativeDrop(g,u,k,tc,a5,a6,a7,a8,a9,a10);return r;}
// Native CB core context: damage reference, monster/player/hireling divisors,
// missile flag. Keep native RNG, target/player-count reductions and resistance.
using CrushingFn=int(__fastcall*)(void*,void*,void*,void*);
inline CrushingFn OriginalCrushing=nullptr;
inline int __fastcall Crushing(void* game,void* attacker,void* target,void* context){
 if(!context)return OriginalCrushing(game,attacker,target,context);
 alignas(8) unsigned char adjusted[24];std::memcpy(adjusted,context,sizeof(adjusted));
 for(unsigned offset:{8u,12u,16u}){int divisor=Read(adjusted,offset);if(divisor>0)Write(adjusted,offset,int(std::min<int64_t>(int64_t(divisor)*5,0x7fffffff)));}
 return OriginalCrushing(game,attacker,target,adjusted);
}
inline bool InstallCrushing(const D2RL::PluginContext* c){
 // Chain the loader's existing CB adapter instead of replacing its integration.
 const auto* entry=reinterpret_cast<const unsigned char*>(c->exeBase+0x581bd0);
 const unsigned char verified[]={0x41,0x8b,0x59,0x08,0x49,0x8b,0xc8};
 if(!c->api->checkExpectedBytes(c,0x581be6,verified,sizeof(verified))||entry[0]!=0xff||entry[1]!=0x25)return false;
 int displacement=0;std::memcpy(&displacement,entry+2,4);
 auto slot=c->exeBase+0x581bd6+displacement;
 OriginalCrushing=*reinterpret_cast<CrushingFn*>(slot);if(!OriginalCrushing)return false;
 return c->api->patchWrite(c,slot-c->exeBase,&OriginalCrushing,8,reinterpret_cast<uint64_t>(Crushing),8);
}
inline bool Install(const D2RL::PluginContext* c){
 if(!InstallCrushing(c))return false;
 const unsigned char resolve[]={0x40,0x55,0x53,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57};
 const unsigned char block[]={0x48,0x89,0x6c,0x24,0x18,0x48,0x89,0x7c,0x24,0x20,0x41,0x56};
 const unsigned char rate[]={0x40,0x55,0x57,0x41,0x57,0x48,0x83,0xec,0x30};
 if(!c->api->checkExpectedBytes(c,0x44df10,resolve,sizeof(resolve))||!c->api->checkExpectedBytes(c,0x44b930,block,sizeof(block))||!c->api->checkExpectedBytes(c,0x347da0,rate,sizeof(rate)))return false;
 // Existing D2RCore resistance adapter remains chained, including Labyrinth.
 const auto* branch=reinterpret_cast<const unsigned char*>(c->exeBase+0x44ec5a);if(branch[0]!=0xe8)return false;int disp=0;std::memcpy(&disp,branch+1,4);auto thunk=c->exeBase+0x44ec5f+disp;
 const auto* t=reinterpret_cast<const unsigned char*>(thunk);if(t[0]!=0xff||t[1]!=0x25)return false;std::memcpy(&disp,t+2,4);auto slot=thunk+6+disp;auto original=*reinterpret_cast<ComponentFn*>(slot);if(!original)return false;OriginalComponent=original;
 auto a=D2RL::MakeInlineHook(0x44df10,resolve,sizeof(resolve),reinterpret_cast<void*>(Resolve),reinterpret_cast<void**>(&OriginalResolve));auto b=D2RL::MakeInlineHook(0x44b930,block,sizeof(block),reinterpret_cast<void*>(Block),reinterpret_cast<void**>(&OriginalBlock));
 if(!c->api->installInlineHook(c,&a)||!c->api->installInlineHook(c,&b))return false;
 if(!c->api->patchWrite(c,slot-c->exeBase,&original,8,reinterpret_cast<std::uint64_t>(Resist),8))return false;
 auto rateHook=D2RL::MakeInlineHook(0x347da0,rate,sizeof(rate),reinterpret_cast<void*>(CappedBlockRate),reinterpret_cast<void**>(&BlockRate));if(!c->api->installInlineHook(c,&rateHook))return false;Moving=reinterpret_cast<MovingFn>(c->exeBase+0x34c390);Dodge=reinterpret_cast<DodgeFn>(c->exeBase+0x44ef70);
 constexpr auto gates=Casts(std::make_index_sequence<172>{});auto table=reinterpret_cast<SkillFn*>(c->exeBase+0x238ea00);
 for(unsigned i=0;i<172;++i)if(table[i]){NativeSkills[i]=table[i];if(!c->api->patchWrite(c,0x238ea00+8*i,&NativeSkills[i],8,reinterpret_cast<std::uint64_t>(gates[i]),8))return false;}
 const D2RL::SharedEventService* events=nullptr;
 if(c->QueryService(&Items)!=D2RL::ServiceQueryResult::Success||c->QueryService(&events)!=D2RL::ServiceQueryResult::Success)return false;
 D2RL::SharedEvents::ItemTooltipListener tooltip{.structSize=D2RL::SharedEvents::ItemTooltipListenerSize,.region=D2RL::SharedEvents::ItemTooltipRegion::Attributes,.position=D2RL::SharedEvents::ItemTooltipPosition::Top,.callback=PrototypeTooltip};
 D2RL::SharedEvents::ListenerHandle handle{};if(events->registerItemTooltipListener(c,&tooltip,&handle)!=D2RL::SharedEvents::Result::Success)return false;
 NativeDrop=MonsterRuntime::OriginalDrop;MonsterRuntime::OriginalDrop=Drop;
 c->LogInfo("PoE Mythicals: conversion, Prototype resistance, lucky block, Aegis barrier, life-scaling spells and bounded returning missiles enabled.");return true;
}
}
