#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
// All runtime records use game/session + unit IDs, never retained unit pointers.
// Item chances reuse two audited obsolete stat IDs; mastery effects share the same engine.
namespace Barbarian {
#include "barbarian_bindings.h"
#include "amazon_melee_bindings.h"
inline int (*TestLevel)(void*,void*,int)=nullptr;
using SetAiParamsFn=void(__fastcall*)(void*,int,int,int);
inline SetAiParamsFn SetAiParams=nullptr;
// Exact native PLAYERPETS_RemovePet ABI, verified at the Unsummon call site.
using RemovePetFn=void(__fastcall*)(void*,void*,unsigned,int);
inline RemovePetFn RemovePet=nullptr;
using UnitWorldFn=void(__fastcall*)(void*,void*);
using ModeInfoFn=void(__fastcall*)(void*,int,void*);
using ModeChangeFn=int(__fastcall*)(void*,void*,int);
using RoomDeleteFn=void(__fastcall*)(void*,unsigned,unsigned);
inline RoomDeleteFn RoomDelete=nullptr;
inline UnitWorldFn DeleteUnit=nullptr;
inline ModeInfoFn ModeInfo=nullptr;
inline ModeChangeFn ModeChange=nullptr;
using PathFn=void*(__fastcall*)(void*);
using StepFn=void(__fastcall*)(void*,unsigned char);
using PetAiFn=void(__fastcall*)(void*,void*,void*);
inline PathFn GetPath=nullptr;
inline StepFn SetStep=nullptr;
inline PetAiFn OriginalPetAi=nullptr;
inline unsigned Depth=0,Last=0,NextScan=0,Random=0x82a173fu,Logs=0,SkillLogs=0,RockLogs=0,RockChildLogs=0;
inline void* Session=nullptr;
inline int Level(void* game,void* owner,int skill){
    if(TestLevel)return TestLevel(game,owner,skill);
    for(const auto& pair:Helpers)if(pair[0]==skill)return std::clamp(Curses::EvalCalc4(game,owner,pair[1],1,0),0,1000);
    return 0;
}
inline int HelperCalc(void* game,void* owner,int skill,int which){
    for(const auto& pair:Helpers)if(pair[0]==skill){void* row=Curses::Record(Ramp::Version(game),pair[1]);if(!row)return 0;
        int formula=0;std::memcpy(&formula,static_cast<unsigned char*>(row)+0x190+4*(which-1),4);
        return Curses::Evaluate(Ramp::Version(game),owner,formula,pair[1],1);}
    return 0;
}
inline unsigned Id(void* u){return static_cast<unsigned*>(u)[2];}
inline int AncientIndex(void* u){if(!Curses::Monster(u))return -1;for(int i=0;i<3;++i)if(static_cast<unsigned*>(u)[1]==AncientClasses[i])return i;return -1;}
inline void* Actor(void* game,void* u){
    if(Curses::Player(u))return static_cast<unsigned*>(u)[1]==4?u:nullptr;
    return AncientIndex(u)>=0?Curses::SkeletonOwner(game,u):nullptr;
}
inline bool Enemy(void* game,void* u){return Curses::Monster(u)&&AuraFeatures::Alive(u)&&Ramp::GetUnit(u,172,0)==0&&!Curses::Player(Curses::SkeletonOwner(game,u));}
inline bool MeleeWeaponSkill(int skill){
    switch(skill){case 0:case 1:case 126:case 132:case 134:case 136:case 137:case 139:case 140:case 141:case 143:case 144:case 147:case 150:case 151:case 152:return true;default:return false;}
}
constexpr int ItemBleedChance=256,ItemImpaleChance=257;
inline int CombinedChance(int mastery,int item){return int(std::clamp<long long>(static_cast<long long>(mastery)+item,0,100));}
// Item modifiers belong to the attacking player. Pets inherit their documented
// Barbarian masteries only, never the owner's equipment proc chances.
inline void* AilmentActor(void* game,void* attacker){return Curses::Player(attacker)?attacker:Actor(game,attacker);}
inline int Chance(int level){return level>0?std::min(100,10+2*level):0;}
inline bool Roll(int chance){Random=Random*1664525u+1013904223u;return chance>0&&int((Random>>8)%100)<chance;}
inline int UsedSkill(void* u){
    void* list=nullptr;std::memcpy(&list,static_cast<unsigned char*>(u)+0x100,8);if(!list)return 0;
    void* used=nullptr;std::memcpy(&used,static_cast<unsigned char*>(list)+0x18,8);if(!used)return 0;
    void* row=nullptr;std::memcpy(&row,used,8);short id=0;if(row)std::memcpy(&id,row,2);return id;
}
struct Impale {int amount=0;unsigned until=0;unsigned hits=0;};
struct Target {
    unsigned id=0xffffffffU,cls=0,owner=0,bleedUntil=0,nextBleed=0,movingUntil=0;
    int bleed=0,x=0,y=0;std::array<Impale,5> impales{};
};
struct Ancient {unsigned id=0xffffffffU,cls=0,owner=0,until=0,next=0,castAt=0,sequenceSince=0;bool expired=false;};
struct HitContext {void* packet=nullptr;unsigned target=0,cls=0,owner=0,frame=0;int base=0;};
inline std::array<Target,2048> Targets{};
inline std::array<Ancient,96> Ancients{};
struct RageTimer {unsigned owner=0xffffffffU,until=0;int level=0;};
inline std::array<RageTimer,16> Rages{};
struct PendingKill {unsigned target=0xffffffffU,cls=0,owner=0,until=0;};
inline std::array<PendingKill,256> PendingKills{};
inline thread_local std::array<HitContext,32> Hits{};
struct StompBurst {unsigned id=0xffffffffU,type=0,cls=0,next=0;int level=1,left=0;};
inline std::array<StompBurst,64> Stomps{};
struct StompCooldown {unsigned id=0xffffffffU,next=0;};
inline std::array<StompCooldown,16> StompCooldowns{};
inline bool StompDispatch=false;
inline unsigned StompLogs=0,TalicStompLogs=0;
using WarcryFn=int(__fastcall*)(void*,void*,int,int);
inline WarcryFn OriginalWarcry=nullptr;

inline void Reset(void* game){unsigned now=Ramp::Frame(game);if(Session!=game||now<Last){Targets={};Ancients={};Rages={};PendingKills={};Hits={};Stomps={};StompCooldowns={};StompDispatch=false;NextScan=0;Session=game;Depth=0;}Last=now;}
inline Target* Track(void* unit,unsigned now){
    Target* free=nullptr;
    for(auto& t:Targets){if(t.id==Id(unit)&&t.cls==static_cast<unsigned*>(unit)[1])return &t;
        bool active=t.bleedUntil>=now;for(auto& i:t.impales)active|=i.hits&&i.until>=now;
        if(!free&&(t.id==0xffffffffU||!active))free=&t;}
    if(free){*free={};free->id=Id(unit);free->cls=static_cast<unsigned*>(unit)[1];free->x=Curses::X(unit);free->y=Curses::Y(unit);}return free;
}

inline int ReadDamage(void* packet,unsigned offset){int value=0;std::memcpy(&value,static_cast<unsigned char*>(packet)+offset,4);return value;}
inline void AddPhysical(void* packet,int amount){
    int physical=ReadDamage(packet,0x18),total=ReadDamage(packet,0x134);
    amount=std::clamp(amount,0,0x3fffffff-std::max(0,std::max(physical,total)));
    physical+=amount;total+=amount;std::memcpy(static_cast<unsigned char*>(packet)+0x18,&physical,4);std::memcpy(static_cast<unsigned char*>(packet)+0x134,&total,4);
}
inline int CarnagePermille(int level,int hp,int maximum){
    if(level<1||maximum<=0)return 0;
    const int missing=std::clamp<int>(int(std::int64_t{maximum-hp}*100/maximum),0,100);
    return missing*(20+2*level);
}
inline void Before(void* game,void* attacker,void* defender,void* packet){
    if(!game||Depth||!packet||!Enemy(game,defender))return;
    Reset(game);void* owner=AilmentActor(game,attacker);if(!Curses::Player(owner))return;
    unsigned short flags=0;std::memcpy(&flags,static_cast<unsigned char*>(packet)+4,2);
    if(!(flags&1)||(flags&0x4020))return;
    for(auto& h:Hits)if(!h.packet){h={packet,Id(defender),static_cast<unsigned*>(defender)[1],Id(owner),Ramp::Frame(game),std::max(0,ReadDamage(packet,0x18))};break;}
}
inline bool StartStomp(void*,void*,int);
inline void Hit(void* game,void* attacker,void* defender,void* packet){
    if(!game||Depth||!packet||!Enemy(game,defender))return;
    Reset(game);unsigned short flags=0;std::memcpy(&flags,static_cast<unsigned char*>(packet)+4,2);
    if(!(flags&1)||(flags&0x4020)||ReadDamage(packet,0x134)<=0)return;
    // Only a successful real melee strike starts a burst. Its own missiles
    // and triggered spells cannot start another burst from the selected skill.
    if(!StompDispatch&&!Curses::ProcDepth&&Curses::Player(attacker)&&UsedSkill(attacker)==140&&!(ReadDamage(packet,0)&2))
        StartStomp(game,attacker,std::max(1,Level(game,attacker,140)));
    // The damage-event hook controls hit reactions; its packet can be a copy.
    // Only this resolved-hit hook precedes the actual native HP subtraction.
    // Scale here once, alongside Attract, so Rage changes the applied damage.
    void* rageOwner=Actor(game,attacker);
    if(Curses::Player(rageOwner)){
        const int permille=CarnagePermille(Level(game,rageOwner,128),Ramp::GetUnit(defender,6,0),Ramp::GetUnit(defender,7,0));
        if(permille>0)AuraFeatures::ScaleResolvedDamage(packet,1000+permille,1000);
    }
    const unsigned now=Ramp::Frame(game);Target* t=nullptr;
    for(auto& target:Targets)if(target.id==Id(defender)&&target.cls==static_cast<unsigned*>(defender)[1]){t=&target;break;}
    const int resolvedBase=std::max(0,ReadDamage(packet,0x18));
    // Every real hit consumes earlier Impales, including allied or spell hits.
    std::int64_t stored=0;if(t)for(auto& impale:t->impales){if(impale.hits&&impale.until>=now){stored+=impale.amount;--impale.hits;}else impale={};}
    if(stored>0){int resist=std::clamp(Ramp::GetUnit(defender,36,0),-100,100);AddPhysical(packet,int(std::min<std::int64_t>(stored*(100-resist)/100,0x1fffffff)));}
    void* owner=AilmentActor(game,attacker);if(!Curses::Player(owner))return;
    const int resist=std::clamp(Ramp::GetUnit(defender,36,0),-100,100);
    int base=resist<100?int(std::min<std::int64_t>(std::int64_t{resolvedBase}*100/(100-resist),0x1fffffff)):0;
    for(const auto& h:Hits)if(h.packet==packet&&h.target==Id(defender)&&h.frame==now){base=h.base;break;}
    if(!base)return;
    const int skill=UsedSkill(attacker);const bool thrown=skill==2||AncientIndex(attacker)==1;
    const int bleedLevel=(Actor(game,attacker)&&!thrown&&(MeleeWeaponSkill(skill)||AncientIndex(attacker)>=0))?Level(game,owner,127):0;
    const int impaleLevel=(Actor(game,attacker)&&(thrown||skill==137))?Level(game,owner,135):0;
    const int itemBleed=Curses::Player(attacker)?Ramp::GetUnit(attacker,ItemBleedChance,0):0;
    const int itemImpale=Curses::Player(attacker)?Ramp::GetUnit(attacker,ItemImpaleChance,0):0;
    const int jabLevel=(skill==10&&Curses::Player(attacker)&&!Curses::ProcDepth&&!(ReadDamage(packet,0)&2))?(TestLevel?TestLevel(game,attacker,10):Curses::EvalCalc4(game,attacker,JabLevelSkill,1,0)):0;
    const bool bleed=Roll(CombinedChance(Chance(bleedLevel)+(jabLevel>0?std::min(100,9+jabLevel):0),itemBleed)),impale=Roll(CombinedChance(Chance(impaleLevel),itemImpale));
    if(!bleed&&!impale)return;if(!t)t=Track(defender,now);if(!t)return;
    if(bleed){const int rate=int(std::min<std::int64_t>(std::int64_t{base}*70/100,0x1fffffff));
        if(t->bleedUntil<now||rate>=t->bleed){t->owner=Id(owner);t->bleed=rate;t->bleedUntil=now+125;t->nextBleed=now+25;}}
    if(impale){Impale* slot=nullptr;for(auto& i:t->impales)if(!i.hits||i.until<now){slot=&i;break;}
        if(!slot)slot=&*std::min_element(t->impales.begin(),t->impales.end(),[](const auto& a,const auto& b){return a.until<b.until;});
        *slot={std::max(1,base/10),now+200,5};}
}
// The native allocator rejects MonStats2.isAtt=0 even for positive buffs.
// These private untargetable summons need a narrowly scoped eligibility window.
// Server-side calls are synchronous: restore the private row before returning;
// never change isSel, unit targeting flags, killability or the damage guard.
inline Curses::RecordFn AncientExtra=nullptr;
struct AncientBuffEligibility {
    unsigned char* flags=nullptr;unsigned char previous=0;
    AncientBuffEligibility(void* game,void* unit){
        if(AncientIndex(unit)<0||!AncientExtra)return;
        void* row=AncientExtra(Ramp::Version(game),int(static_cast<unsigned*>(unit)[1]));
        if(row){flags=static_cast<unsigned char*>(row)+5;previous=*flags;*flags|=2;}
    }
    ~AncientBuffEligibility(){if(flags)*flags=previous;}
    AncientBuffEligibility(const AncientBuffEligibility&)=delete;
    AncientBuffEligibility& operator=(const AncientBuffEligibility&)=delete;
};
inline void* ApplyFriendlyBuff(void* game,Curses::Curse& request){
    AncientBuffEligibility eligibility(game,request.target);
    return Curses::OriginalApply(&request);
}
inline void* Buff(void* game,void* owner,int skill,int level,int state,unsigned duration){
    void* list=Ramp::OriginalList(owner,state);
    if(!list){Curses::Curse request{owner,owner,skill,level,int(duration),-1,0,state,nullptr};list=ApplyFriendlyBuff(game,request);}
    if(list){unsigned until=Ramp::Frame(game)+duration;Ramp::SetExpiry(list,int(until));Ramp::Schedule(game,owner,12,int(until),0,0,0);if(SoulHarvest::MarkGfxDirty)SoulHarvest::MarkGfxDirty(owner,state,1);}
    return list;
}
inline void GrantCarnage(void* game,void* owner){
    int level=Level(game,owner,128);if(level<1)return;
    const unsigned until=Ramp::Frame(game)+250;
    RageTimer* timer=nullptr;for(auto& entry:Rages)if(entry.owner==Id(owner)){timer=&entry;break;}
    if(!timer)for(auto& entry:Rages)if(entry.owner==0xffffffffU){timer=&entry;break;}
    if(timer)*timer={Id(owner),until,level};
    if(void* list=Buff(game,owner,128,level,CarnageState,250)){
        Curses::Write(game,list,68,20+level);if(SoulHarvest::Refresh)SoulHarvest::Refresh(owner);Curses::Update(owner,"Carnage",0);}
}
// Fixed twenty-subtile radius (13 1/3 displayed yards).
inline double StompRadius(int){return 20.0;}
inline int StompDamagePercent(int level,int meleeHard=0,int throwHard=0){
    // 50% total base weapon damage, +10 percentage points per level. Each mastery adds
    // 5% increased pulse damage per hard point, with no multiplicative stacking.
    return int(std::min<std::int64_t>(std::int64_t{50+10*std::clamp(level,1,1000)}*(100+5*(std::clamp(meleeHard,0,1000)+std::clamp(throwHard,0,1000)))/100,0x3fffffff));
}
inline int StompVisualRange(int level){return int(std::ceil(StompRadius(level)*4.0/3.0));}
inline int StompCount(int level){return 5+std::clamp(level,1,1000)/10;}
inline bool StartStomp(void* game,void* caster,int level){
    if(!game||!caster||StompDispatch||!AuraFeatures::Alive(caster)||!Curses::Player(caster))return false;
    Reset(game);
    StompCooldown* cooldown=nullptr;
    for(auto& c:StompCooldowns)if(c.id==Id(caster)){cooldown=&c;break;}
    if(!cooldown)for(auto& c:StompCooldowns)if(c.id==0xffffffffU){cooldown=&c;break;}
    const auto now=Ramp::Frame(game);if(!cooldown||(cooldown->id==Id(caster)&&now<cooldown->next))return false;
    for(auto& burst:Stomps)if(burst.id==0xffffffffU){
        burst={Id(caster),static_cast<unsigned*>(caster)[0],static_cast<unsigned*>(caster)[1],Ramp::Frame(game),std::clamp(level,1,1000),StompCount(level)};*cooldown={Id(caster),now+125};return true;
    }
    return false;
}
inline void StompPulse(void* game,void* caster,int level){
    if(!Pulse::OriginalCreate||!Curses::NativeMissileHit)return;
    const bool previous=StompDispatch;StompDispatch=true;
    // Visuals are native War Cry rings, with all collision and damage removed.
    // Gameplay has one explicit native hit per target, independent of ring density.
    // Create the real War Cry shockwave sprites directly: the native shout
    // helper may reject an unlearned private visual skill on summoned units.
    constexpr int directions[16][2]={{100,0},{92,38},{71,71},{38,92},{0,100},{-38,92},{-71,71},{-92,38},{-100,0},{-92,-38},{-71,-71},{-38,-92},{0,-100},{38,-92},{71,-71},{92,-38}};
    for(const auto& direction:directions){Pulse::Params ring{};ring.flags=0x8021;ring.owner=caster;ring.missile=RockControl;ring.skill=RainSkill;ring.level=level;ring.x=Curses::X(caster);ring.y=Curses::Y(caster);ring.targetX=ring.x+direction[0];ring.targetY=ring.y+direction[1];ring.range=StompVisualRange(level);Pulse::OnCreate(game,&ring);}

    std::array<unsigned,256> targets{};unsigned count=0;
    const double radius=StompRadius(level);void* room=SoulHarvest::Room(caster);
    AuraFeatures::Units(game,1,[&](void* target){
        if(count<targets.size()&&Enemy(game,target)&&SoulHarvest::Room(target)==room&&GuardianCombat::Distance(caster,target)<=radius*radius)targets[count++]=Id(target);
    });
    for(unsigned i=0;i<count;++i){
        void* target=Curses::Find(game,1,targets[i]);if(!Enemy(game,target))continue;
        Pulse::Params hit{};hit.flags=0x8021;hit.owner=caster;hit.origin=target;hit.target=target;
        hit.missile=RockHit;hit.skill=140;hit.level=level;hit.x=Curses::X(target)-1;hit.y=Curses::Y(target);hit.targetX=hit.x+2;hit.targetY=hit.y;hit.range=1;
        // The complete native missile handler supplies physical resistance,
        // stun eligibility, weapon modifiers, hit events and normal kill credit.
        // Do not set global ProcDepth: ordinary on-striking effects must work.
        if(void* missile=Pulse::OnCreate(game,&hit))Curses::NativeMissileHit(game,missile,target,0);
    }
    StompDispatch=previous;
    if(Ramp::Log&&StompLogs++<20){char text[128]{};std::snprintf(text,sizeof(text),"Ground Stomp pulse: caster=%u level=%d targets=%u radius=%.1f",Id(caster),level,count,radius);Ramp::Log(text);}
}
inline void StompTick(void* game,unsigned now){
    for(auto& burst:Stomps){
        if(burst.id==0xffffffffU||now<burst.next)continue;
        void* caster=Curses::Find(game,burst.type,burst.id);
        if(!AuraFeatures::Alive(caster)||static_cast<unsigned*>(caster)[1]!=burst.cls){burst={};continue;}
        // No catch-up loop after a delayed frame: a burst remains five distinct
        // hits, with at least five server frames between consecutive pulses.
        StompPulse(game,caster,burst.level);
        if(--burst.left==0)burst={};else burst.next=now+5;
    }
}
inline bool ProjectileContext(void*) noexcept{return false;}
inline bool FilterProjectile(void*,void* raw) noexcept{
    auto* p=static_cast<Pulse::Params*>(raw);
    if(p&&p->skill==RainSkill&&p->missile==RockControl){p->flags|=0x8000U;p->range=StompVisualRange(p->level);}
    return true;
}
inline int QuakeWeaponPercent(int level,int hardPoints){
    return int(std::int64_t{50+5*(std::clamp(level,1,1000)-1)}*(100+5*std::clamp(hardPoints,0,1000))/300);
}
inline int QuakePoints(void* game,void* owner,int level){
    if(!owner)return 0;void* row=Curses::Record(Ramp::Version(game),137);if(!row)return 0;
    int formula=0;std::memcpy(&formula,static_cast<unsigned char*>(row)+0x1a0,4);
    return Curses::Evaluate(Ramp::Version(game),owner,formula,137,level);
}
inline void ProjectileCreated(void* game,void* raw,void* missile) noexcept{
    const auto* p=static_cast<Pulse::Params*>(raw);if(!p||!missile)return;
    if(p->skill==137&&p->missile==QuakeMissile){
        // Native SrcDamage=128 captures the complete weapon snapshot without
        // flat skill damage. Scale damage only, retaining duration/proc stats.
        const int hardPoints=std::clamp(QuakePoints(game,p->owner,p->level),0,1000);
        for(int stat:{21,22,48,49,50,51,52,53,54,55,57,58,316,317}){
            // Divide only after scaling to retain the exact one-third weapon share.
            const auto damage=std::int64_t{Ramp::GetUnit(missile,stat,0)}*(50+5*(std::clamp(p->level,1,1000)-1))*(100+5*hardPoints)/30000;
            const int flat=stat==21?1+std::clamp(p->level,1,1000):stat==22?2+std::clamp(p->level,1,1000):0;
            const auto added=std::int64_t{flat}*256*(100+5*hardPoints)/100;
            Ramp::SetUnit(missile,stat,int(std::clamp<std::int64_t>(damage+added,-0x3fffffff,0x3fffffff)),0);
        }
    }
    if(p->missile==RockHit){
        // Keep the complete native weapon snapshot, then scale only damage.
        // Durations, leech percentages and proc metadata retain their meaning.
        const int damage=StompDamagePercent(p->level,p->owner?HelperCalc(game,p->owner,127,5):0,p->owner?HelperCalc(game,p->owner,135,5):0);
        for(int stat:{21,22,48,49,50,51,52,53,54,55,57,58,316,317}){
            const int value=Ramp::GetUnit(missile,stat,0);
            if(value)Ramp::SetUnit(missile,stat,int(std::clamp<std::int64_t>(std::int64_t{value}*damage/100,-0x3fffffff,0x3fffffff)),0);
        }
    }
    if(p->missile==RockControl||(p->owner&&AncientIndex(p->owner)==1)){
        SoulHarvest::CachePlayerIds(game);
        for(unsigned i=0;i<SoulHarvest::PlayerCount;++i)
            if(void* player=Curses::Find(game,0,SoulHarvest::PlayerIds[i]))SoulHarvest::SyncOrbToPlayer(game,missile,player);
    }
}
inline bool IgnoreDamage(void*,void*,void* defender,void*){return AncientIndex(defender)>=0;}
inline void After(void* game,void* attacker,void* defender,void* packet){
    if(!game||Depth||!packet)return;
    unsigned short flags=0;std::memcpy(&flags,static_cast<unsigned char*>(packet)+4,2);
    for(auto& h:Hits)if(h.packet==packet){const auto saved=h;h={};
        void* dead=Curses::Find(game,1,saved.target);void* owner=Curses::Find(game,0,saved.owner);
        if(!dead||static_cast<unsigned*>(dead)[1]!=saved.cls||!Curses::Player(owner))return;
        // This event hook runs before native HP subtraction on normal attacks.
        // Confirm death on the following server tick instead of discarding it.
        if(Ramp::GetUnit(dead,6,0)<=0){GrantCarnage(game,owner);return;}
        PendingKill* pending=nullptr;for(auto& k:PendingKills)if(k.target==saved.target&&k.cls==saved.cls){pending=&k;break;}
        if(!pending)for(auto& k:PendingKills)if(k.target==0xffffffffU||k.until<Ramp::Frame(game)){pending=&k;break;}
        if(pending)*pending={saved.target,saved.cls,saved.owner,Ramp::Frame(game)+25};
        return;
    }
}
inline void BleedTick(void* game,void* owner,void* target,int fixed){
    if(!Pulse::OriginalCreate||!Curses::NativeMissileHit||fixed<1)return;
    const int old=Ramp::GetUnit(owner,AuraBuffs::ChargeStat,0);
    const int amount=std::clamp((fixed+255)/256,1,10000000);
    ++Depth;++Curses::ProcDepth;Ramp::SetUnit(owner,AuraBuffs::ChargeStat,amount,0);
    Pulse::Params p{};p.flags=0x8000;p.owner=owner;p.origin=target;p.target=target;p.missile=TickMissile;p.skill=TickSkill;p.level=1;p.x=Curses::X(target)-1;p.y=Curses::Y(target);p.targetX=p.x+2;p.targetY=p.y;p.range=1;
    void* missile=Pulse::OriginalCreate(game,&p);Ramp::SetUnit(owner,AuraBuffs::ChargeStat,old,0);
    if(missile){const unsigned id=Id(target);Curses::NativeMissileHit(game,missile,target,0);
        void* dead=Curses::Find(game,1,id);if(dead&&Ramp::GetUnit(dead,6,0)<=0)GrantCarnage(game,owner);}
    --Curses::ProcDepth;--Depth;
}
inline int __fastcall Warcry(void* game,void* owner,int skill,int level){
    if(skill!=131)return OriginalWarcry(game,owner,skill,level);
    if(!Curses::Player(owner)||!AuraFeatures::Alive(owner))return 0;
    Reset(game);std::array<unsigned,128> corpses{};unsigned count=0;
    void* room=SoulHarvest::Room(owner);
    AuraFeatures::Units(game,1,[&](void* u){if(count==corpses.size()||(Curses::OriginalCheck&&Curses::OriginalCheck(u,RedeemedState))||Ramp::GetUnit(u,6,0)>0||static_cast<unsigned*>(u)[3]!=12||AncientIndex(u)>=0||SoulHarvest::Room(u)!=room)return;
        if(GuardianCombat::Distance(u,owner)<=12*12)corpses[count++]=Id(u);});
    unsigned consumed=0;
    for(unsigned id=0;id<count&&consumed<5;++id){void* corpse=Curses::Find(game,1,corpses[id]);if(!corpse)continue;
        if(RedemptionLife::Original(game,owner,corpse,131,level,1))++consumed;}
    if(!consumed)return 0;
    int maximum=std::max(0,Ramp::GetUnit(owner,7,0)),life=Ramp::GetUnit(owner,6,0);
    const int heal=int(std::min<std::int64_t>(std::int64_t{maximum}*(3+level/5)*consumed/100,maximum));
    Ramp::SetUnit(owner,6,int(std::min<std::int64_t>(std::int64_t{life}+heal,maximum)),0);
    if(void* list=Buff(game,owner,131,level,FeastState,(8+level/5+2*(consumed-1))*25))Curses::Write(game,list,18,20+3*level);
    if(SoulHarvest::Refresh)SoulHarvest::Refresh(owner);Curses::Update(owner,"Feast of War",0);return 1;
}
// Renew an independent active marker even when no corpse is in range. The
// damage buff keeps its own short timer; toggling off releases reservation.
inline bool FeastPulse(void* game,void* owner,unsigned index,int level){
    if(AuraBuffs::Definitions[index].skill!=131)return false;
    const unsigned duration=std::max(51U,AuraBuffs::Next(game,owner,index,level)-Ramp::Frame(game)+2);
    Buff(game,owner,131,level,FeastAuraState,int(duration));
    Warcry(game,owner,131,level);
    return true;
}
inline void Applied(void* raw){
    auto* c=static_cast<Curses::Curse*>(raw);
    if(!c||c->skill!=146||!Curses::Player(c->caster)||!Curses::Monster(c->target)||Depth||!Curses::Cast)return;
    ++Depth;Curses::Cast(c->caster,PullSkill,c->level,c->target,0);--Depth;
}
inline void AncientBuff(void* game,void* owner,void* pet,int index){
    const int vigor=Level(game,owner,148),melee=Level(game,owner,127),throwing=Level(game,owner,135);
    void* list=Buff(game,pet,CallSkill,1,AncientPassivesState,100);if(!list)return;
    // Match the correct weapon mastery, without copying the owner's equipment.
    if(index==1){const int meleeDmg=melee?28+5*(melee-1):0,throwDmg=throwing?28+6*(throwing-1):0;
        Curses::Write(game,list,18,throwDmg-meleeDmg);
        const int meleeAr=melee?40+8*(melee-1):0,throwAr=throwing?44+8*(throwing-1):0;Curses::Write(game,list,119,throwAr-meleeAr);}
    Curses::Write(game,list,105,vigor?10+2*vigor:0);Curses::Write(game,list,102,vigor?10+2*vigor:0);
    int crit=index==1?HelperCalc(game,owner,135,6):(melee?10+HelperCalc(game,owner,127,5):0);
    Curses::Write(game,list,141,std::clamp(crit,0,100));
    if(index==1)Curses::Write(game,list,166,std::clamp(HelperCalc(game,owner,135,7),0,100));
    if(SoulHarvest::Refresh)SoulHarvest::Refresh(pet);
}
inline D2RL::Reimplementations::GetLevelIdFromUnitFn AuraArea=nullptr;
inline unsigned AuraLogs=0,AuraReports=0,AuraReportAt=0;
inline bool SameAuraArea(void* a,void* b){
    // One level spans many internal rooms. Compare areas before coordinates.
    if(AuraArea){const int level=AuraArea(a);return level>0&&level==AuraArea(b);}
    return SoulHarvest::Room(a)&&SoulHarvest::Room(a)==SoulHarvest::Room(b);
}
// Native basic auras omit these untargetable pets. Reuse the caster's computed
// aura values, not equipment/aggregate stats, and the native state (no stacking).
// Row offsets are verified against BasicAura65 in this supported D2R build.
inline void ShareAura(void* game,void* owner,unsigned index,int level){
    const auto& aura=AuraBuffs::Definitions[index];
    const bool diagnose=aura.skill==98&&Ramp::Log&&Ramp::Frame(game)>=AuraReportAt&&AuraReports<60;
    if(diagnose){++AuraReports;AuraReportAt=Ramp::Frame(game)+125;char text[160]{};std::snprintf(text,sizeof(text),"Ancient aura checkpoint: owner=%u function=%d level=%d state=%d",Id(owner),aura.function,level,aura.state);Ramp::Log(text);}
    if(aura.function!=65||AuraBuffs::PassiveOrSummon(aura.skill)||!Curses::Player(owner))return;
    void* row=Curses::Record(Ramp::Version(game),aura.skill);
    void* source=Ramp::OriginalList(owner,aura.state);
    if(diagnose){char text[120]{};std::snprintf(text,sizeof(text),"Ancient aura source: row=%d list=%d",row!=nullptr,source!=nullptr);Ramp::Log(text);}
    if(!row||!source)return;
    auto* raw=static_cast<unsigned char*>(row);
    unsigned filter=0;std::memcpy(&filter,raw+0x70,4);
    if(!(filter&0x10000))return; // Never transmit hostile or self-only effects.
    short state=-1;std::memcpy(&state,raw+0xa2,2);if(state!=aura.state)return;
    int formula=0;std::memcpy(&formula,raw+0x84,4);
    const int radius=std::clamp(Curses::Evaluate(Ramp::Version(game),owner,formula,aura.skill,level),0,1000);
    if(diagnose){char text[180]{};std::snprintf(text,sizeof(text),"Ancient aura range: formula=%d radius=%d area=%d",formula,radius,AuraArea?AuraArea(owner):-1);Ramp::Log(text);}
    if(!radius)return;
    std::array<short,6> stats{};std::memcpy(stats.data(),raw+0x74,sizeof(stats));
    const unsigned duration=std::max(51U,AuraBuffs::Next(game,owner,index,level)-Ramp::Frame(game)+2);
    AuraFeatures::Units(game,1,[&](void* pet){
        if(AncientIndex(pet)<0)return;
        if(diagnose){char text[180]{};std::snprintf(text,sizeof(text),"Ancient aura target: pet=%u alive=%d owned=%d area=%d distance2=%lld",Id(pet),AuraFeatures::Alive(pet),Curses::SkeletonOwner(game,pet)==owner,AuraArea?AuraArea(pet):-1,static_cast<long long>(GuardianCombat::Distance(owner,pet)));Ramp::Log(text);}
        if(!AuraFeatures::Alive(pet)||Curses::SkeletonOwner(game,pet)!=owner)return;
        if(!SameAuraArea(owner,pet)||GuardianCombat::Distance(owner,pet)>std::int64_t{radius}*radius)return;
        void* list=Ramp::OriginalList(pet,state);
        if(list&&Curses::Caster(game,list)!=owner){
            // Preserve native arbitration of another caster's aura; never
            // take ownership of it or add a second copy of the same state.
            return;
        }
        if(!list){Curses::Curse request{owner,pet,aura.skill,level,int(duration),-1,0,state,nullptr};list=ApplyFriendlyBuff(game,request);}
        if(!list){if(diagnose)Ramp::Log("Ancient aura allocation failed");return;}
        for(int stat:stats)if(stat>=0)Curses::Write(game,list,stat,Curses::Read(game,source,stat));
        Ramp::SetExpiry(list,int(Ramp::Frame(game)+duration));
        Ramp::Schedule(game,pet,12,int(Ramp::Frame(game)+duration),0,0,0);
        if(SoulHarvest::Refresh)SoulHarvest::Refresh(pet);
        if(GuardianCombat::ToggleState)GuardianCombat::ToggleState(pet,state,1);
        if(SoulHarvest::MarkGfxDirty)SoulHarvest::MarkGfxDirty(pet,state,1);
        Curses::Update(pet,"Ancient allied aura",0);
        if(Ramp::Log&&AuraLogs++<12){char text[180]{};std::snprintf(text,sizeof(text),"Ancient aura applied: pet=%u skill=%d level=%d state=%d damage=%d crushingBlow=%d radius=%d",Id(pet),aura.skill,level,state,Curses::Read(game,list,25),Curses::Read(game,list,136),radius);Ramp::Log(text);}
    });
}
inline bool SequenceBusy(void* pet){
    if(!pet)return false;
    const unsigned mode=static_cast<unsigned*>(pet)[3];
    return mode==7||mode==8||mode==9||mode==10||mode==13||mode==14;
}
inline void __fastcall PetAi(void* game,void* pet,void* tick){
    // The verified GenericPet callback runs even while a monster sequence is
    // active. Its follow/attack decisions were replacing Korlic's jump with
    // walking and clearing Talic's skill. Defer only these three private pets;
    // keep the sequence and its native movement/animation events untouched.
    if(AncientIndex(pet)>=0&&SequenceBusy(pet)&&AuraFeatures::Alive(pet)){
        if(Ramp::Schedule)Ramp::Schedule(game,pet,2,int(Ramp::Frame(game)+5),0,0,0);
        return;
    }
    if(OriginalPetAi)OriginalPetAi(game,pet,tick);
}
inline void AncientAttack(void* game,void* owner,void* pet,int index,Ancient& slot,unsigned now){
    if(!Curses::Cast||now<slot.castAt||!AuraFeatures::Alive(pet))return;
    void* target=nullptr;auto best=std::int64_t{28*28};
    AuraFeatures::Units(game,1,[&](void* u){if(!Enemy(game,u)||SoulHarvest::Room(u)!=SoulHarvest::Room(pet))return;auto d=GuardianCombat::Distance(u,pet);if(d<best){best=d;target=u;}});
    if(!target)return;
    const int level=std::max(1,Level(game,owner,CallSkill));
    if(index!=1&&!GuardianCombat::InMelee(pet,target))return;
    // Respect native attack sequences. A second proc must not replace
    // their used-skill node before the engine's own sequence has finished.
    const unsigned mode=static_cast<unsigned*>(pet)[3];
    if(mode==7||mode==8||mode==9||mode==10||mode==13||mode==14){
        if(index!=1&&slot.sequenceSince&&now>slot.sequenceSince+100&&ModeInfo&&ModeChange){
            alignas(8) unsigned char idle[0x80]{};ModeInfo(pet,1,idle);ModeChange(game,idle,1);slot.sequenceSince=0;slot.castAt=now+5;
        }
        return;
    }
    if(!ModeInfo||!ModeChange){slot.castAt=now+10;return;}
    // The native triggered-skill dispatcher executes one do-function only. Leap
    // and Whirlwind require the real monster sequence and its animation events.
    alignas(8) unsigned char change[0x80]{};
    ModeInfo(pet,14,change);
    GuardianCombat::SkillScope selected(pet,AncientAttacks[index]);
    if(!selected.valid){slot.castAt=now+10;return;}
    void* aim=target;std::memcpy(change+0x10,&aim,8);
    int x=Curses::X(target),y=Curses::Y(target);
    std::memcpy(change+0x18,&x,4);std::memcpy(change+0x1c,&y,4);
    const int started=ModeChange(game,change,0);
    if(Ramp::Log&&SkillLogs++<24){char text[150]{};std::snprintf(text,sizeof(text),"Ancient native skill: pet=%u index=%d skill=%d started=%d mode=%u",Id(pet),index,AncientAttacks[index],started,static_cast<unsigned*>(pet)[3]);Ramp::Log(text);}
    if(started){
        if(selected.used)selected.previous=*selected.used;
        slot.sequenceSince=now;slot.castAt=now+(index==1?15:25);
    }else{
        slot.castAt=now+10;
    }
}
inline void Tick(void* game){
    if(!game||!Curses::Find||!Curses::X||!Curses::Y||!SoulHarvest::Room)return;
    Reset(game);const unsigned now=Ramp::Frame(game);if(now<NextScan)return;NextScan=now+5;
    StompTick(game,now);
    for(auto& pending:PendingKills){if(pending.target==0xffffffffU)continue;
        void* unit=Curses::Find(game,1,pending.target);
        if(unit&&static_cast<unsigned*>(unit)[1]==pending.cls&&Ramp::GetUnit(unit,6,0)<=0){
            void* owner=Curses::Find(game,0,pending.owner);pending={};if(Curses::Player(owner))GrantCarnage(game,owner);
        }else if(!unit||now>pending.until)pending={};
    }
    for(auto& timer:Rages){if(timer.owner==0xffffffffU)continue;void* unit=Curses::Find(game,0,timer.owner);
        if(!AuraFeatures::Alive(unit)||now>=timer.until){timer={};continue;}
        if(void* list=Buff(game,unit,128,timer.level,CarnageState,timer.until-now)){
            Curses::Write(game,list,68,20+timer.level);if(SoulHarvest::Refresh)SoulHarvest::Refresh(unit);Curses::Update(unit,"Rage duration",0);
        }
    }
    for(auto& t:Targets){if(t.id==0xffffffffU)continue;void* u=Curses::Find(game,1,t.id);
        if(!Enemy(game,u)||static_cast<unsigned*>(u)[1]!=t.cls){t={};continue;}
        const int x=Curses::X(u),y=Curses::Y(u);if(x!=t.x||y!=t.y)t.movingUntil=now+10;t.x=x;t.y=y;
        if(t.bleedUntil>=now&&t.bleed&&now>=t.nextBleed){void* owner=Curses::Find(game,0,t.owner);
            if(Curses::Player(owner)){const int damage=int(std::min<std::int64_t>(std::int64_t{t.bleed}*(t.movingUntil>now?3:1),0x1fffffff));BleedTick(game,owner,u,damage);}t.nextBleed=now+25;}
        if(t.bleedUntil<now)t.bleed=0;
        bool active=t.bleed>0;for(auto& i:t.impales){if(i.until<now)i={};active|=i.hits>0;}if(!active)t={};
    }
    AuraFeatures::Units(game,1,[&](void* pet){const int index=AncientIndex(pet);if(index<0||!AuraFeatures::Alive(pet))return;
        void* owner=Curses::SkeletonOwner(game,pet);if(!Curses::Player(owner))return;
        Ancient* slot=nullptr;for(auto& a:Ancients)if(a.id==Id(pet)&&a.cls==static_cast<unsigned*>(pet)[1]){slot=&a;break;}
        if(!slot)for(auto& a:Ancients)if(a.id==0xffffffffU){slot=&a;a={Id(pet),static_cast<unsigned*>(pet)[1],Id(owner),now+unsigned((30+Level(game,owner,CallSkill))*25),0};
            // Hydra initializes its own AI timeout. The Ancient AI needs its
            // regular zeroed parameters; our ID-based lifetime handles expiry.
            if(SetAiParams){void* data=nullptr;std::memcpy(&data,static_cast<unsigned char*>(pet)+0x10,8);void* ai=nullptr;if(data)std::memcpy(&ai,static_cast<unsigned char*>(data)+0x30,8);if(ai)SetAiParams(ai,0,0,0);}
            if(Ramp::Log&&Logs++<18){char message[144]{};std::snprintf(message,sizeof(message),"Ancient tracked: id=%u class=%u owner=%u expires=%u",a.id,a.cls,a.owner,a.until);Ramp::Log(message);}break;}
        if(!slot)return;
        if(slot->expired)return;
        if(now>=slot->until||!AuraFeatures::Alive(owner)){
            if(RemovePet&&RoomDelete&&DeleteUnit){
                const unsigned id=Id(pet);slot->expired=true;
                // Detach without trying to kill an immortal monster, then use
                // the engine's own delete queue + world removal exactly once.
                ++Depth;
                if(void* room=SoulHarvest::Room(pet))RoomDelete(room,1,id);
                RemovePet(game,owner,id,0);DeleteUnit(game,pet);--Depth;
                if(Ramp::Log&&Logs++<24)Ramp::Log("Ancient permanently removed at expiry");
            }return;}
        if(now>=slot->next){AncientBuff(game,owner,pet,index);slot->next=now+25;}
        AncientAttack(game,owner,pet,index,*slot,now);
    });
    for(auto& a:Ancients)if(a.id!=0xffffffffU){void* p=Curses::Find(game,1,a.id);if(!p||!AuraFeatures::Alive(p)||static_cast<unsigned*>(p)[1]!=a.cls)a={};}
}
inline bool Install(const D2RL::PluginContext* context){
    AuraArea=reinterpret_cast<D2RL::Reimplementations::GetLevelIdFromUnitFn>(GetProcAddress(GetModuleHandleW(L"D2RCore.dll"),"GetLevelIdFromUnit"));
    const unsigned char extraBytes[]={0x48,0x89,0x5c,0x24,0x08,0x55,0x56,0x57,0x48,0x83,0xec,0x30,0x48,0x63,0xf2};
    if(!context->api->checkExpectedBytes(context,0x975b0,extraBytes,sizeof(extraBytes)))return false;
    AncientExtra=reinterpret_cast<Curses::RecordFn>(context->exeBase+0x975b0);
    const unsigned char aiLayout[]={0x48,0x8b,0x43,0x10,0x48,0x8b,0x48,0x30};
    const unsigned char aiCall[]={0xe8,0x47,0xe2,0xf4,0xff};
    if(!context->api->checkExpectedBytes(context,0x5543fb,aiLayout,sizeof(aiLayout))||
       !context->api->checkExpectedBytes(context,0x554414,aiCall,sizeof(aiCall)))return false;
    const unsigned char removeCall[]={0xe8,0x33,0xf7,0x0c,0x00};
    if(!context->api->checkExpectedBytes(context,0x4305f8,removeCall,sizeof(removeCall)))return false;
    const auto table=reinterpret_cast<void**>(context->exeBase+0x238ea00);
    if(table[4]!=reinterpret_cast<void*>(context->exeBase+0x4305c0))return false;
    const unsigned char shoutBytes[]={0x48,0x89,0x5c,0x24,0x10,0x48,0x89,0x6c,0x24,0x18,0x48,0x89,0x74,0x24,0x20,0x57,0x41,0x56,0x41,0x57};
    if(table[68]!=reinterpret_cast<void*>(context->exeBase+0x567500)||!context->api->checkExpectedBytes(context,0x567500,shoutBytes,sizeof(shoutBytes)))return false;
    auto stompHook=D2RL::MakeInlineHook(0x567500,shoutBytes,sizeof(shoutBytes),reinterpret_cast<void*>(Warcry),reinterpret_cast<void**>(&OriginalWarcry));
    if(!context->api->installInlineHook(context,&stompHook))return false;
    const unsigned char petAiBytes[]={0x40,0x55,0x53,0x57,0x48,0x8d,0xac,0x24,0x70,0xfe,0xff,0xff,0x48,0x81,0xec,0x90,0x02,0x00,0x00};
    const unsigned char pathBytes[]={0x40,0x53,0x48,0x83,0xec,0x20,0x48,0x8b,0xd9,0x48,0x85,0xc9};
    const unsigned char stepBytes[]={0x48,0x85,0xc9,0x74,0x13,0xfe,0xca,0x33,0xc0,0x0f,0xb6,0xd2};
    if(!context->api->checkExpectedBytes(context,0x5d2f70,petAiBytes,sizeof(petAiBytes))||
       !context->api->checkExpectedBytes(context,0x34ae80,pathBytes,sizeof(pathBytes))||
       !context->api->checkExpectedBytes(context,0x342890,stepBytes,sizeof(stepBytes)))return false;
    GetPath=reinterpret_cast<PathFn>(context->exeBase+0x34ae80);
    SetStep=reinterpret_cast<StepFn>(context->exeBase+0x342890);
    auto petHook=D2RL::MakeInlineHook(0x5d2f70,petAiBytes,sizeof(petAiBytes),reinterpret_cast<void*>(PetAi),reinterpret_cast<void**>(&OriginalPetAi));
    if(!context->api->installInlineHook(context,&petHook))return false;
    const unsigned char modeInfoBytes[]={0x48,0x89,0x5c,0x24,0x20,0x55,0x56,0x57,0x48,0x83,0xec,0x20};
    const unsigned char modeChangeBytes[]={0x40,0x53,0x55,0x56,0x41,0x55,0x48,0x83,0xec,0x38};
    const unsigned char roomDeleteCall[]={0xe8,0xfc,0x0d,0xd9,0xff};
    const unsigned char deleteBytes[]={0x48,0x89,0x74,0x24,0x18,0x57,0x48,0x83,0xec,0x20};
    if(!context->api->checkExpectedBytes(context,0x4471e0,modeInfoBytes,sizeof(modeInfoBytes))||
       !context->api->checkExpectedBytes(context,0x4475c0,modeChangeBytes,sizeof(modeChangeBytes))||
       !context->api->checkExpectedBytes(context,0x55e6cf,roomDeleteCall,sizeof(roomDeleteCall))||
       !context->api->checkExpectedBytes(context,0x48faa0,deleteBytes,sizeof(deleteBytes)))return false;
    ModeInfo=reinterpret_cast<ModeInfoFn>(context->exeBase+0x4471e0);
    ModeChange=reinterpret_cast<ModeChangeFn>(context->exeBase+0x4475c0);
    RoomDelete=reinterpret_cast<RoomDeleteFn>(context->exeBase+0x2ef4d0);
    DeleteUnit=reinterpret_cast<UnitWorldFn>(context->exeBase+0x48faa0);
    RemovePet=reinterpret_cast<RemovePetFn>(context->exeBase+0x4ffd30);
    SetAiParams=reinterpret_cast<SetAiParamsFn>(context->exeBase+0x4a2660);
    Curses::ExtraInstantMissile=TickMissile;Curses::ExtraInstantMissile2=RockHit;Curses::ExtraStackSkill=PullSkill;
    context->LogInfo("Barbarian overhaul enabled: Rage, Feast, Quake, Ancients, Bleeding and Impale");return true;
}
} // namespace Barbarian
