#pragma once
#include <array>
#include <algorithm>
#include <cstdint>
#include <vector>
#include <utility>
namespace MonsterModifiers {
using Id=std::uint64_t;using Mask=std::uint32_t;
constexpr Id Invalid=~Id(0);
// Native unit IDs overlap between players and monsters. Persist the type too.
constexpr Id Identity(unsigned type,unsigned id){return (Id(type)<<32)|id;}
constexpr unsigned Type(Id id){return unsigned(id>>32);}
constexpr unsigned Number(Id id){return unsigned(id);}

enum Mod:unsigned {Strong,Fast,Cursed,Gifted,Fire,Lightning,Cold,Mana,Teleport,Spectral,Stone,Multishot,Aura,Poison,Gravity,Mirror,Leader,Glitter,Raid,Totems,Duelist,Ritualist,CorpseEater,Count};
constexpr Mask Bit(Mod m){return Mask(1)<<unsigned(m);}
constexpr Mask All=(Mask(1)<<Count)-1;
constexpr std::array<int,13> NativeIds={5,6,7,8,9,17,18,25,26,27,28,29,30};
inline Mask NativeMask(const unsigned char* ids,unsigned n=9){Mask result=0;if(ids)for(unsigned i=0;i<n&&ids[i];++i)for(unsigned j=0;j<NativeIds.size();++j)if(ids[i]==NativeIds[j])result|=Mask(1)<<j;return result;}
inline Mask Expand(Mask mask){return (mask&Bit(Raid))?All:mask;}
inline unsigned ModifierCount(Mask mask){unsigned n=0;for(mask=Expand(mask);mask;mask&=mask-1)++n;return n;}
inline int DamagePercent(Mask mask){return 100+5*int(ModifierCount(mask));}
inline int LifePercent(Mask mask){return 100+25*int(ModifierCount(mask));}
inline std::pair<int,int> RollRange(unsigned difficulty){return difficulty==0?std::pair<int,int>{1,2}:difficulty==1?std::pair<int,int>{2,4}:std::pair<int,int>{3,5};}
inline int MissileDamage(int whole,int element){return int(std::int64_t(std::clamp(whole,0,1000000))*256/(element==5?25:1));}
inline int LootRolls(Mask m){return m&Bit(Raid)?5:m&Bit(Glitter)?2:1;}
// Both reductions multiply the resolved damage, not resistances. Totems do
// not stack with one another; Stone Skin may still combine with their aura.
inline int DamageTaken(int value,Mask mask,bool totem){std::int64_t v=std::max(0,value);if(mask&Bit(Stone))v=v*25/100;if(totem)v=v*10/100;return int(v);}
inline unsigned Random(unsigned& seed){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;}
inline Mod Roll(Mask eligible,Mask used,unsigned& seed){
 eligible&=~Expand(used);unsigned weight=0;for(unsigned i=0;i<Count;++i)if(eligible&(Mask(1)<<i))weight+=i==Raid?1:6;
 if(!weight)return Count;unsigned r=Random(seed)%weight;
 for(unsigned i=0;i<Count;++i)if(eligible&(Mask(1)<<i)){unsigned w=i==Raid?1:6;if(r<w)return Mod(i);r-=w;}return Count;
}
// Arena checks use subtiles (12 subtiles = 8 yards). Include the boundary.
constexpr int ArenaRadius=12;
// Distances use native subtiles: 24 = 16 displayed yards. Pull two thirds
// toward the source, then let native relocation reject blocked destinations.
constexpr int GravityRadius=24;
inline int GravityDestination(int from,int center){return from+2*(center-from)/3;}
inline bool InsideArena(int dx,int dy){return std::int64_t(dx)*dx+std::int64_t(dy)*dy<=ArenaRadius*ArenaRadius;}
inline int RitualLifePercent(unsigned stones){return stones>=3?400:stones==2?200:100;}
inline int RitualDamagePercent(unsigned stones,Mask parent=0){return (stones>=3?300:stones==2?200:125)*DamagePercent(parent)/100;}
struct Unit {Id id=Invalid;int level=1,x=0,y=0;bool alive=false;};
enum class Action {FireBall,LightningBolt,IceBolt,ChainLightning,BurningGround,DeathOrb,FireExplosion,PoisonPool,GravityWarning,GravityPull,GravityExplosion,TeleportStrike,SpectralFire,SpectralCold,SpectralLightning,SpectralMagic,SpectralPoison,MirrorImage,PackMember,Totem,Arena,RuneStone,RitualSummon,ConsumeCorpse};
struct Event {Action action;Id owner,target;int x,y,damage;unsigned duration;};
struct Host {
 virtual ~Host()=default;
 virtual bool get(Id,Unit&)=0;
 virtual Id nearestEnemy(Id,int)=0;
 virtual void emit(const Event&)=0;
 virtual unsigned liveChildren(Id,Action)=0;
};
struct Actor {Id id=Invalid;Mask mask=0;unsigned seed=1,nextGifted=0,nextLightning=0,nextCold=0,nextTeleport=0,nextPoison=0,nextGravity=0,gravityAt=0,nextMirror=0,nextLeader=0,nextTotem=0,nextArena=0,nextCorpse=0,ritualAt=0,lastHit=0xffffffffu;int hits=0;bool died=false,ritualStarted=false;};
struct Delayed {Id owner=Invalid;Action action=Action::FireExplosion;int x=0,y=0,damage=0;unsigned due=0;};
struct Engine {
 std::vector<Actor> actors;std::vector<Delayed> pending;
 void clear(){actors.clear();pending.clear();}
 Actor* find(Id id){for(auto& a:actors)if(a.id==id)return &a;return nullptr;}
 Actor* add(Id id,Mask mask,unsigned now){if(auto* a=find(id)){a->mask=Expand(a->mask|mask);return a;}if(actors.size()>=8192)return nullptr;
 Actor a;a.id=id;a.mask=Expand(mask);a.seed=id^0x91cba721u;if(!a.seed)a.seed=1;
 a.nextGifted=a.nextLightning=a.nextCold=now+25;a.nextTeleport=now+100;a.nextPoison=now+25;a.nextGravity=now+25;a.nextMirror=a.nextLeader=now+25;a.nextTotem=now+75;actors.push_back(a);return &actors.back();}
 // Snapshot work before emitting: native casts may synchronously trigger other
 // events and allocate records. Never keep references across Host::emit.
 void hit(Host& h,Id owner,Id target,unsigned now,bool direct){auto* a=find(owner);if(!a||a->died||!direct)return;Unit u;if(!h.get(owner,u)||!u.alive)return;std::vector<Event> events;
 if(a->mask&Bit(Fire)){Unit t;if(h.get(target,t))events.push_back({Action::BurningGround,owner,target,t.x,t.y,5+u.level,75});}
 if((a->mask&Bit(Spectral))&&++a->hits%4==0){auto kind=Action(unsigned(Action::SpectralFire)+unsigned(a->hits/4-1)%5);Unit t;if(h.get(target,t))events.push_back({kind,owner,target,t.x,t.y,15+4*u.level,0});}
 for(const auto& e:events)h.emit(e);
 }
 void death(Host& h,Id owner,unsigned now){auto* a=find(owner);if(!a||a->died)return;Unit u;if(!h.get(owner,u))return;a->died=true;std::vector<Event> events;
 if(a->mask&Bit(Fire)){events.push_back({Action::DeathOrb,owner,Invalid,u.x,u.y,0,75});if(pending.size()<4096)pending.push_back({owner,Action::FireExplosion,u.x,u.y,100+30*u.level,now+75});}
 if(a->mask&Bit(Poison))for(int i=0;i<3;++i)events.push_back({Action::PoisonPool,owner,Invalid,u.x+(i-1)*3,u.y+(i==1?3:0),10+2*u.level,75});
 for(const auto& e:events)h.emit(e);
 }
 void tick(Host& h,unsigned now){std::vector<Event> events;
 for(auto& a:actors){if(a.died)continue;Unit u;if(!h.get(a.id,u)||!u.alive)continue;
 auto emit=[&](Action kind,Id target,int damage=0,unsigned duration=0){events.push_back({kind,a.id,target,u.x,u.y,damage,duration});};
 Id target=h.nearestEnemy(a.id,30);
 // Resolve at ten seconds even if the player has moved out of targeting range.
 // Snapshot stone count before native emit can create or destroy helper units.
 if(a.ritualAt&&now>=a.ritualAt){a.ritualAt=0;emit(Action::RitualSummon,target,int(std::min(3u,h.liveChildren(a.id,Action::RuneStone))),1500);}
 if((a.mask&Bit(CorpseEater))&&now>=a.nextCorpse){a.nextCorpse=now+75;emit(Action::ConsumeCorpse,target);}
 // Keep the visual boundary independent of AI targeting; overlap renewals.
 if((a.mask&Bit(Duelist))&&now>=a.nextArena){a.nextArena=now+20;emit(Action::Arena,Invalid,0,40);}
 // No catch-up loops after a pause: each mechanic emits at most once per tick.
 if(target!=Invalid){
 if((a.mask&Bit(Ritualist))&&!a.ritualStarted){a.ritualStarted=true;a.ritualAt=now+250;for(int stone=0;stone<3;++stone)emit(Action::RuneStone,target,stone,275);}
 if((a.mask&Bit(Gifted))&&now>=a.nextGifted){a.nextGifted=now+25;emit(Action(unsigned(Action::FireBall)+Random(a.seed)%3),target,30+8*u.level);}
 if((a.mask&Bit(Lightning))&&now>=a.nextLightning){a.nextLightning=now+25;emit(Action::ChainLightning,target,15+4*u.level);}
 if((a.mask&Bit(Cold))&&now>=a.nextCold){a.nextCold=now+25;emit(Action::IceBolt,target,15+4*u.level);}
 if((a.mask&Bit(Teleport))&&now>=a.nextTeleport){a.nextTeleport=now+100;emit(Action::TeleportStrike,target);}
 if((a.mask&Bit(Gravity))&&now>=a.nextGravity){a.nextGravity=now+125;a.gravityAt=now+25;emit(Action::GravityWarning,target,0,25);}
 if(a.gravityAt&&now>=a.gravityAt){a.gravityAt=0;emit(Action::GravityPull,target);emit(Action::GravityExplosion,target,20+5*u.level);}
 if((a.mask&Bit(Poison))&&now>=a.nextPoison){a.nextPoison=now+50;emit(Action::PoisonPool,target,10+2*u.level,75);}
 if((a.mask&Bit(Mirror))&&now>=a.nextMirror){a.nextMirror=now+200;unsigned alive=h.liveChildren(a.id,Action::MirrorImage);for(unsigned i=alive;i<2;++i)emit(Action::MirrorImage,target,0,150);}
 if((a.mask&Bit(Leader))&&now>=a.nextLeader){a.nextLeader=now+200;if(h.liveChildren(a.id,Action::PackMember)<4)emit(Action::PackMember,target);}
 if((a.mask&Bit(Totems))&&now>=a.nextTotem){a.nextTotem=now+75;if(h.liveChildren(a.id,Action::Totem)<3)emit(Action::Totem,target,0,250);}
 }
 }
 for(auto& d:pending)if(d.owner!=Invalid&&now>=d.due){events.push_back({d.action,d.owner,Invalid,d.x,d.y,d.damage,0});d.owner=Invalid;}
 pending.erase(std::remove_if(pending.begin(),pending.end(),[](const Delayed& d){return d.owner==Invalid;}),pending.end());
 for(const auto& e:events)h.emit(e);
 }
};
}
