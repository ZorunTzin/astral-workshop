#pragma once
#include "src/curses_native.h"
// Loaded after aura_buffs.h. Reuse its single frame hook and the curse plugin's
// guarded, post-resistance hit hook; never install competing hooks on them.
namespace AuraFeatures {
namespace GuardianIds {
#include "guardian_bindings.h"
}
using namespace AuraBuffs;
constexpr unsigned GroundBuffFrames=8*25,StillFrames=25;
constexpr int GroundRadius=12; // Native subtiles: 12 * 2/3 = 8 displayed yards.
struct Source {
    unsigned id=0xffffffffU,last=0,stillSince=0,nextGround=0,groundUntil=0,nextVisual=0;
    int x=0,y=0,groundX=0,groundY=0,hits=0,pendingNovas=0;
    bool initialized=false;
};
inline std::array<Source,16> Sources{};
inline void* Session=nullptr;
inline unsigned LastFrame=0;
inline Source* Track(void* game,void* unit) {
    const unsigned now=Ramp::Frame(game);
    if(Session!=game||now<LastFrame){Sources={};Session=game;}
    LastFrame=now;
    const unsigned id=static_cast<unsigned*>(unit)[2];
    for(auto& s:Sources)if(s.id==id)return &s;
    for(auto& s:Sources)if(s.id==0xffffffffU){s={};s.id=id;return &s;}
    return nullptr;
}
inline bool Enabled(void* unit,int skill) {
    const int i=Index(skill);
    const unsigned id=static_cast<unsigned*>(unit)[2];
    for(const auto& p:Players)if(p.id==id)return p.costs[i]>0;
    return false;
}
inline int ActiveLevel(void* game,void* unit,int skill) {
    return Enabled(unit,skill)?Level(game,unit,Index(skill)):0;
}
inline int Effect(void* unit){return std::clamp(Ramp::GetUnit(unit,EffectStat,0),0,10000);}
inline int Scale(int value,void* unit){return static_cast<int>(std::int64_t{value}*(100+Effect(unit))/100);}
inline int DamageBonus(int level,void* unit){return Scale(20+5*level,unit)/2;}
inline int RegenBasisPoints(int level,void* unit){return Scale(300+20*level,unit)/2;}
// Fixed physical damage replaces damage storage. Snapshot only while native
// missiles are created: each missile then owns its damage independently.
inline int NovaDamage(int level,void* unit){return Scale(50+25*std::clamp(level,1,1000),unit);}
inline int NovaRadiusPercent(int level,void* unit){return Scale(100+3*(std::clamp(level,1,1000)-1),unit);}
inline thread_local bool EmittingNova=false;
inline bool Pulse(void* game,void* unit,unsigned index,int level) {
    const int skill=Definitions[index].skill;
    if(skill!=103&&skill!=109)return false;
    // The expensive work has its own fixed cadence, independent of native aura
    // pulses; activation succeeds immediately and reserves mana like any toggle.
    Track(game,unit);
    // The custom retaliation action replaces native Thorns. Keep only an empty
    // timed state as an active-buff indicator; no reflected-damage stat is added.
    if(skill==103&&Curses::OriginalApply){
        const int state=Definitions[index].state;
        const unsigned now=Ramp::Frame(game),duration=std::max(51U,Next(game,unit,index,level)-now+2);
        void* list=Ramp::OriginalList(unit,state);
        if(!list){Curses::Curse request{unit,unit,skill,level,static_cast<int>(duration),-1,0,state,nullptr};list=Curses::OriginalApply(&request);}
        if(list){Ramp::SetExpiry(list,static_cast<int>(now+duration));Ramp::Schedule(game,unit,12,static_cast<int>(now+duration),0,0,0);}
    }
    return true;
}
inline void Disable(void*,void* unit,unsigned index) {
    const unsigned id=static_cast<unsigned*>(unit)[2];
    for(auto& s:Sources)if(s.id==id) {
        if(Definitions[index].skill==103){s.hits=0;s.pendingNovas=0;}
        if(Definitions[index].skill==109){s.initialized=false;s.groundUntil=0;}
    }
}
inline bool Alive(void* u){return u&&(Curses::Player(u)||Curses::Monster(u))&&!Dead(u);}
inline void ApplyGround(void* game,void* owner,void* target,int level) {
    if(!Alive(target))return;
    const int bonus=DamageBonus(level,owner);
    void* list=Ramp::OriginalList(target,GroundState);
    if(list&&Curses::Read(game,list,GroundPhysicalStat)>bonus&&Curses::Caster(game,list)!=owner)return;
    const unsigned until=Ramp::Frame(game)+GroundBuffFrames;
    if(list) {
        Ramp::SetExpiry(list,static_cast<int>(until));
        Ramp::Schedule(game,target,12,static_cast<int>(until),0,0,0);
    } else {
        Curses::Curse request{owner,target,109,level,static_cast<int>(GroundBuffFrames),-1,0,GroundState,nullptr};
        list=Curses::OriginalApply(&request);
    }
    if(!list)return;
    // Native elemental and magic mastery affects spells, weapon elemental
    // damage and poison. Physical damage has no universal native spell mastery,
    // so only that component is increased once in the resolved hit callback.
    Curses::Write(game,list,GroundPhysicalStat,bonus);
    for(int stat:{329,330,331,332,357})Curses::Write(game,list,stat,bonus);
    const auto regen=std::int64_t{std::max(0,Ramp::GetUnit(target,7,0))}*RegenBasisPoints(level,owner)/250000;
    Curses::Write(game,list,74,static_cast<int>(std::min<std::int64_t>(regen,0x3fffffff)));
    if(SoulHarvest::Refresh)SoulHarvest::Refresh(target);
    if(SoulHarvest::MarkGfxDirty)SoulHarvest::MarkGfxDirty(target,GroundState,1);
    Curses::Update(target,"Sanctified Ground",0);
    if(!Curses::RegenPending(game,target))Ramp::Schedule(game,target,3,static_cast<int>(Ramp::Frame(game)+1),0,0,0);
}
// Native unit hash arrays: verified player bucket at 0x2230, monster bucket at
// 0x2630 and next-unit link at 0x158. Traverse once per ground pulse, never per
// damage hit or render frame. Bound links to fail closed on malformed chains.
template<class F> inline void Units(void* game,int type,F callback) {
    auto* raw=static_cast<unsigned char*>(game);
    for(unsigned bucket=0;bucket<128;++bucket) {
        void* unit=nullptr;std::memcpy(&unit,raw+(type?0x2630:0x2230)+8*bucket,8);
        for(unsigned links=0;unit&&links<4096;++links) {
            void* next=nullptr;std::memcpy(&next,static_cast<unsigned char*>(unit)+0x158,8);
            callback(unit);unit=next;
        }
    }
}
// Skills.txt exposes six aura-stat columns. Salvation already uses all six
// for its three resistances and three maxima; add poison maximum to the same
// native timed aura list after its ordinary pulse, without another buff timer.
inline void AfterPulse(void* game,void* owner,unsigned index,int level){
    if(Definitions[index].skill!=125)return;
    const int bonus=Scale(1+std::clamp(level,1,1000)/10,owner);
    auto apply=[&](void* unit){
        if(!Alive(unit))return;void* list=Ramp::OriginalList(unit,Definitions[index].state);if(!list)return;
        void* caster=Curses::Caster(game,list);if(caster&&caster!=owner)return;
        Curses::Write(game,list,46,bonus);
        if(SoulHarvest::Refresh)SoulHarvest::Refresh(unit);
        if(SoulHarvest::MarkGfxDirty)SoulHarvest::MarkGfxDirty(unit,Definitions[index].state,1);
    };
    Units(game,0,apply);Units(game,1,apply);
}
inline void GroundTargets(void* game,void* owner,Source& source,int level) {
    void* room=SoulHarvest::Room?SoulHarvest::Room(owner):nullptr;
    auto receive=[&](void* target) {
        if(!Alive(target))return;
        if(Curses::Monster(target)&&!Curses::Player(Curses::SkeletonOwner(game,target)))return;
        // Room identity prevents matching coordinates in another act/dungeon.
        if(room&&SoulHarvest::Room(target)!=room)return;
        const auto dx=std::int64_t{Curses::X(target)}-source.groundX,dy=std::int64_t{Curses::Y(target)}-source.groundY;
        if(dx*dx+dy*dy<=GroundRadius*GroundRadius)ApplyGround(game,owner,target,level);
    };
    Units(game,0,receive);Units(game,1,receive);
}
inline void GroundVisual(void* game,void* owner,Source& source) {
    if(!::Pulse::OriginalCreate)return;
    ::Pulse::Params p{};p.flags=0x8021;p.owner=owner;p.origin=owner;p.missile=GroundVisualMissile;
    p.x=source.groundX;p.y=source.groundY;p.targetX=p.x+1;p.targetY=p.y;
    p.skill=109;p.level=1;p.range=static_cast<int>(std::min(100U,source.groundUntil-Ramp::Frame(game)));
    void* visual=::Pulse::OriginalCreate(game,&p);
    if(!visual)return;
    SoulHarvest::CachePlayerIds(game);
    for(unsigned i=0;i<SoulHarvest::PlayerCount;++i)
        if(void* player=Curses::Find(game,0,SoulHarvest::PlayerIds[i]))SoulHarvest::SyncOrbToPlayer(game,visual,player);
}
inline void GroundTick(void* game,void* unit,Source& s,int level) {
    const unsigned now=Ramp::Frame(game);const int x=Curses::X(unit),y=Curses::Y(unit);
    if(!s.initialized||x!=s.x||y!=s.y) {
        s.initialized=true;s.x=x;s.y=y;s.stillSince=now;
    }
    if(now-s.stillSince>=StillFrames) {
        if(s.groundUntil<=now||s.groundX!=x||s.groundY!=y)s.nextVisual=now;
        s.groundX=x;s.groundY=y;s.groundUntil=now+GroundBuffFrames;
    }
    if(s.groundUntil>now&&now>=s.nextGround) {
        GroundTargets(game,unit,s,level);s.nextGround=now+10;
    }
    if(s.groundUntil>now&&now>=s.nextVisual) {
        GroundVisual(game,unit,s);s.nextVisual=now+100;
    }
}
// Scale resolved hostile-monster damage once, including elemental spells.
// Friendly summons/mercenaries and players retain their normal damage.
inline void ScaleResolvedDamage(void* damage,int numerator,int denominator) {
    auto* bytes=static_cast<unsigned char*>(damage);
    for(unsigned offset:{0x18u,0x20u,0x24u,0x2cu,0x30u,0x34u,0x38u,0x134u}) {
        int value=0;std::memcpy(&value,bytes+offset,4);
        if(value>0){value=static_cast<int>(std::min<std::int64_t>(std::int64_t{value}*numerator/denominator,0x3fffffff));std::memcpy(bytes+offset,&value,4);}
    }
}
inline void MonsterDamage(void* game,void* attacker,void* damage) {
    // The private guardian's complete outgoing hit is divided once: weapon,
    // spell, item and aura damage retain the same proportions.
    if(Curses::Monster(attacker)&&static_cast<unsigned*>(attacker)[1]==GuardianIds::MonsterClass){ScaleResolvedDamage(damage,1,10);return;}
    if(!Curses::Monster(attacker)||Ramp::GetUnit(attacker,172,0)!=0)return;
    if(Curses::Player(Curses::SkeletonOwner(game,attacker)))return;
    const unsigned difficulty=*(static_cast<unsigned char*>(game)+0x104);
    if(difficulty>2)return;
    constexpr int factors[]={200,175,150};
    ScaleResolvedDamage(damage,factors[difficulty],100);
}
inline void __fastcall Hit(void* game,void* attacker,void* defender,void* damage) {
    if(!game||!attacker||!defender||attacker==defender||!damage)return;
    MonsterDamage(game,attacker,damage);
    auto* raw=static_cast<unsigned char*>(damage);
    unsigned short flags=0;int total=0;
    std::memcpy(&flags,raw+4,2);std::memcpy(&total,raw+0x134,4);
    if(total<=0||(flags&0x3b0))return;
    const int bonus=std::clamp(Ramp::GetUnit(attacker,GroundPhysicalStat,0),0,10000);
    if(bonus>0) {
        int physical=0;std::memcpy(&physical,raw+0x18,4);
        const int extra=static_cast<int>(std::min<std::int64_t>(std::int64_t{std::max(0,physical)}*bonus/100,std::numeric_limits<int>::max()-std::max(physical,total)));
        physical+=extra;total+=extra;
        std::memcpy(raw+0x18,&physical,4);std::memcpy(raw+0x134,&total,4);
    }
 }
// Observe native attack attempts once, before block/dodge/zero-damage filtering.
// Do not trigger from reflected damage or let our own Nova recurse.
inline void __fastcall IncomingHit(void* game,void* attacker,void* defender,void* damage) {
    if(!game||!attacker||!damage||attacker==defender||EmittingNova||
       !Curses::Player(defender)||!Alive(defender))return;
    unsigned short flags=0;std::memcpy(&flags,static_cast<unsigned char*>(damage)+4,2);
    if(flags&0x4000)return;
    if(ActiveLevel(game,defender,103)<1)return;
    Source* s=Track(game,defender);if(!s)return;
    if(++s->hits==5){s->hits=0;++s->pendingNovas;}
}

inline void Tick(void* game) {
    if(!game)return;
    for(auto& player:Players) {
        if(player.id==0xffffffffU)continue;
        void* unit=Curses::Find(game,0,player.id);
        if(!unit)continue;
        Source* s=Track(game,unit);if(!s)continue;
        if(!Alive(unit)){s->hits=0;s->pendingNovas=0;continue;}
        const int ground=ActiveLevel(game,unit,109);
        if(ground>0)GroundTick(game,unit,*s,ground);
        const int retribution=ActiveLevel(game,unit,103);
        if(retribution<1){s->hits=0;s->pendingNovas=0;continue;}
        if(s->pendingNovas>0&&Curses::Cast) {
            // One release per server frame bounds burst work without discarding
            // earned Novas or imposing a cooldown between sets of five hits.
            Ramp::SetUnit(unit,ChargeStat,NovaDamage(retribution,unit),0);
            EmittingNova=true;
            const int cast=Curses::Cast(unit,NovaSkill,retribution,unit,0);
            EmittingNova=false;
            Ramp::SetUnit(unit,ChargeStat,0,0);
            if(cast)--s->pendingNovas;
        }
    }
}
} // namespace AuraFeatures
#include "guardian_combat.h"
#include "redemption_life.h"
#include "set_selectors.h"
#include "barbarian.h"
#include "sorceress.h"
#include "assassin.h"
#include "amazon.h"
#include "druid.h"
#include "../monster_modifiers/spells.h"
#include "../monster_modifiers/runtime.h"
#include "../endgame/runtime.h"
#include "../goblins/runtime.h"
#include "../leagues/runtime.h"
#include "../mythicals/runtime.h"
#include "../leagues/effects.h"
namespace AuraBuffs {
inline bool InstallFeatures(const D2RL::PluginContext* context) {
    const unsigned char expected[]={0x41,0x8b,0xc0,0x83,0xe0,0x7f};
    // The compiler's actual bucket prologue is mov eax,r8d (41 8b c0).
    if(!context->api->checkExpectedBytes(context,0x4925fe,expected,sizeof(expected)))return false;
    if(!context->api->checkExpectedBytes(context,CurseNative::ReachRva,CurseNative::ReachBytes,sizeof(CurseNative::ReachBytes))||
       !context->api->checkExpectedBytes(context,CurseNative::InRangeRva,CurseNative::InRangeBytes,sizeof(CurseNative::InRangeBytes)))return false;
    Curses::Reach=reinterpret_cast<Curses::ReachFn>(context->exeBase+CurseNative::ReachRva);
    Curses::InRange=reinterpret_cast<Curses::InRangeFn>(context->exeBase+CurseNative::InRangeRva);
    const unsigned char skillSlot[]={0x48,0x85,0xc9,0x75,0x03,0x33,0xc0,0xc3,0x48,0x8b,0x41,0x18,0xc3};
    const unsigned char toggle[]={0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x6c,0x24,0x18,0x48,0x89,0x74,0x24,0x20,0x57};
    if(!context->api->checkExpectedBytes(context,0x33e090,skillSlot,sizeof(skillSlot))||
       !context->api->checkExpectedBytes(context,0x3354c0,toggle,sizeof(toggle)))return false;
    GuardianCombat::BindNativeSkills=true;
    GuardianCombat::ToggleState=reinterpret_cast<GuardianCombat::ToggleStateFn>(context->exeBase+0x3354c0);
    if(!RedemptionLife::Install(context))return false;
    if(!Barbarian::Install(context)||!Assassin::Install(context)||!Amazon::Install(context))return false;
    if(!MonsterModifierSpells::Bind(context)||!MonsterModifiersNative::Install(context)||!MonsterRuntime::Install(context))return false;
    if(!Arena::Install(context)||!Goblins::Install(context))return false;
    if(!League::install(context))context->LogWarn("League encounters disabled: unsupported object API.");
    FeatureFrame=[](void* game){LeagueEffects::Frame(game);AuraFeatures::Tick(game);GuardianCombat::Tick(game);RedemptionLife::Tick(game);SetSelectors::Tick(game);Barbarian::Tick(game);Sorceress::Tick(game);Assassin::Frame(game);Amazon::Frame(game);Druid::Frame(game);MonsterRuntime::Frame(game);Arena::Frame(game);Goblins::Frame(game);League::frame(game);Mythicals::Frame(game);};
    CustomCast=[](void* g,void* u,int s,int l){int d=Druid::Cast(g,u,s,l);if(d>=0)return d;int r=Amazon::Cast(g,u,s,l);return r<0?Assassin::Cast(g,u,s,l):r;};
    Curses::BeforeMissileTick=Amazon::BeforeTick;Curses::AfterMissileTick=Amazon::FinishTick;
    Druid::NativeRaven=reinterpret_cast<SkillFn*>(context->exeBase+0x238ea00)[114];
    Sorceress::NativeMorph=reinterpret_cast<SkillFn*>(context->exeBase+0x238ea00)[116];
    Sorceress::NativeStorm=reinterpret_cast<SkillFn*>(context->exeBase+0x238ea00)[102];
    FeaturePulse=[](void* g,void* u,unsigned i,int l){return Druid::BuffPulse(g,u,i,l)||Amazon::BuffPulse(g,u,i,l)||Assassin::BuffPulse(g,u,i,l)||Sorceress::Pulse(g,u,i,l)||Barbarian::FeastPulse(g,u,i,l)||AuraFeatures::Pulse(g,u,i,l);};FeatureDisable=[](void* g,void* u,unsigned i){Druid::Disable(g,u,i);Assassin::Disable(g,u,i);Sorceress::Disable(g,u,i);AuraFeatures::Disable(g,u,i);};
    Curses::AdditionalHitEffects=[](void* g,void* a,void* d,void* p){if(LeagueEffects::Depth)return;LeagueEffects::Hit(g,a,d,p);AuraFeatures::Hit(g,a,d,p);Barbarian::Hit(g,a,d,p);Sorceress::Hit(g,a,d,p);Assassin::HitEvent(g,a,d,p);Amazon::Hit(g,a,d,p);Druid::Hit(g,a,d,p);Mythicals::Hit(g,a,d,p);MonsterModifiersNative::Damage(g,a,d,p);MonsterRuntime::Damage(g,a,d,p);Arena::Damage(g,a,d,p);};
    Curses::AdditionalIncomingHit=[](void* g,void* a,void* d,void* p){LeagueEffects::Before(g,a,d,p);AuraFeatures::IncomingHit(g,a,d,p);Barbarian::Before(g,a,d,p);};
    Curses::AdditionalMissileContext=Barbarian::ProjectileContext;
    Curses::AdditionalMissileFilter=[](void* g,void* p) noexcept {return MonsterModifierSpells::BeforeCreate(g,p)&&MonsterRuntime::Projectile(g,p)&&Amazon::Filter(g,p)&&Barbarian::FilterProjectile(g,p);};
    Curses::AdditionalMissileCreated=[](void* g,void* p,void* m) noexcept {Barbarian::ProjectileCreated(g,p,m);Sorceress::Projectile(g,p,m);Assassin::Projectile(g,p,m);Amazon::Created(g,p,m);Druid::Projectile(g,p,m);MonsterModifierSpells::Created(g,p,m);};
    Curses::AdditionalAfterDamage=[](void* g,void* a,void* d,void* p){Arena::AfterDamage(g,a,d,p);Barbarian::After(g,a,d,p);MonsterRuntime::After(g,a,d,p);};
    Curses::AdditionalIgnoreDamage=[](void* game,void* attacker,void* defender,void* packet){return Arena::IgnoreDamage(game,attacker,defender,packet)||Barbarian::IgnoreDamage(game,attacker,defender,packet)||MonsterRuntime::IgnoreDamage(game,attacker,defender,packet);};
    Curses::AdditionalWarCry=Barbarian::Applied;
    AfterNativePulse=[](void* g,void* u,unsigned i,int l){Barbarian::ShareAura(g,u,i,l);AuraFeatures::AfterPulse(g,u,i,l);};
    context->LogInfo("Monster phase 1 enabled: Extra Fast +200%, halved Magic Resistant bonus, extra elemental casts, multiplicative Stone Skin; champion variants unchanged");
    context->LogInfo("Druid 0.19.0 enabled: three reservations, native stone morph/ravens, cone/seed spells and companion effects");
    context->LogInfo("Amazon 0.19.0 enabled: four reservations, bounded split/return arrows, Pounce and javelin patterns");
    context->LogInfo("Paladin replacements enabled: efficiency, mastery, guardian, Retribution, Sanctified Ground");
    return true;
}
} // namespace AuraBuffs
