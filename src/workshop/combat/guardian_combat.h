#pragma once
namespace GuardianCombat {
#include "guardian_bindings.h"
using ToggleStateFn=void(__fastcall*)(void*,int,int);
inline ToggleStateFn ToggleState=nullptr;
inline bool BindNativeSkills=false;
// A freshly created private pet has learned skills but no currently used skill.
// The native start/do dispatcher requires that context even for triggered casts.
// Bind the actual learned node only for this call, then restore the AI's slot.
struct SkillScope {
    void** used=nullptr;void* previous=nullptr;bool valid=false;
    SkillScope(void* unit,int skill) {
        if(!BindNativeSkills){valid=true;return;}
        void* list=nullptr;std::memcpy(&list,static_cast<unsigned char*>(unit)+0x100,8);
        if(!list)return;
        void* node=nullptr;std::memcpy(&node,list,8);
        for(unsigned count=0;node&&count<256;++count) {
            void* record=nullptr;std::memcpy(&record,node,8);
            short id=-1;if(record)std::memcpy(&id,record,2);
            if(id==skill) {
                used=reinterpret_cast<void**>(static_cast<unsigned char*>(list)+0x18);
                previous=*used;*used=node;valid=true;return;
            }
            std::memcpy(&node,static_cast<unsigned char*>(node)+8,8);
        }
    }
    ~SkillScope(){if(used)*used=previous;}
    SkillScope(const SkillScope&)=delete;
};
inline int Cast(void* pet,int skill,int level,void* target) {
    SkillScope selected(pet,skill);
    return selected.valid?Curses::Cast(pet,skill,level,target,0):0;
}
inline void Appearance(void* pet) {
    if(!ToggleState||!Curses::OriginalCheck||Curses::OriginalCheck(pet,PaladinState))return;
    ToggleState(pet,93,0); // Native Valkyrie summon always installs Amazon state 93.
    ToggleState(pet,PaladinState,1);
    if(SoulHarvest::Refresh)SoulHarvest::Refresh(pet);
    if(SoulHarvest::MarkGfxDirty){SoulHarvest::MarkGfxDirty(pet,93,0);SoulHarvest::MarkGfxDirty(pet,PaladinState,1);}
    Curses::Update(pet,"Astral Guardian Paladin appearance",0);
}
constexpr unsigned ScanInterval=10; // One bounded world scan each second.
constexpr int CastRange=24; // Native subtiles, sixteen displayed yards.
struct Caster {unsigned id=0xffffffffU,next=0,seen=0,sequence=0,hitAt=0,target=0;int remaining=0,level=0;};
inline std::array<Caster,64> Casters{};
inline void* Session=nullptr;
inline unsigned LastFrame=0,NextScan=0,Logs=0,ZealLogs=0;
inline Caster* Track(void* pet,unsigned now) {
    const unsigned id=static_cast<unsigned*>(pet)[2];
    for(auto& c:Casters)if(c.id==id){c.seen=now;return &c;}
    for(auto& c:Casters)if(c.id==0xffffffffU||now>c.seen+250){c={};c.id=id;c.seen=now;return &c;}
    return nullptr;
}
inline std::int64_t Distance(void* a,void* b) {
    const auto dx=std::int64_t{Curses::X(a)}-Curses::X(b),dy=std::int64_t{Curses::Y(a)}-Curses::Y(b);
    return dx*dx+dy*dy;
}
inline bool Hostile(void* game,void* target) {
    return AuraFeatures::Alive(target)&&Curses::Monster(target)&&
        Ramp::GetUnit(target,172,0)==0&&!Curses::Player(Curses::SkeletonOwner(game,target));
}
inline bool InMelee(void* pet,void* target) {
    if(Curses::Reach&&Curses::InRange)return Curses::InRange(pet,target,Curses::Reach(pet),nullptr)!=0;
    return Distance(pet,target)<=36;
}
inline int Select(int characterLevel,std::int64_t distance,unsigned sequence,bool melee=false) {
    // Prioritize a periodic targeted strike, use the orbiting hammer nearby,
    // and keep a reliable ranged bolt as the baseline. Only combat skills appear.
    if(characterLevel>=CombatSpells[3][1]&&(melee||(!Curses::InRange&&distance<=36))&&sequence%2==0)return 3;
    if(characterLevel>=CombatSpells[2][1]&&sequence%3==0)return 2;
    if(characterLevel>=CombatSpells[1][1]&&distance<=81)return 1;
    return characterLevel>=CombatSpells[0][1]?0:-1;
}
inline void ZealAnimation(void* game,void* pet,int level,int hits) {
    if(!Curses::OriginalApply)return;
    const int duration=hits*5+5;
    void* list=Ramp::OriginalList(pet,ZealState);
    if(!list) {
        Curses::Curse request{pet,pet,CombatSpells[3][0],level,duration,-1,0,ZealState,nullptr};
        list=Curses::OriginalApply(&request);
    }
    if(!list)return;
    // Triggered melee hits do not start a player animation loop. Temporarily
    // speed the pet's real weapon swings, then let the native timed list expire.
    Curses::Write(game,list,68,100);
    Ramp::SetExpiry(list,static_cast<int>(Ramp::Frame(game)+duration));
    Ramp::Schedule(game,pet,12,static_cast<int>(Ramp::Frame(game)+duration),0,0,0);
    if(SoulHarvest::Refresh)SoulHarvest::Refresh(pet);
    if(SoulHarvest::MarkGfxDirty)SoulHarvest::MarkGfxDirty(pet,ZealState,1);
    Curses::Update(pet,"Guardian Zeal attack animation",0);
}
inline int ZealHits(int level){return std::clamp(1+level,2,5);}
inline void Burst(void* game,Caster& slot,unsigned now) {
    if(slot.remaining<1||now<slot.hitAt)return;
    // Keep only IDs across frames. Re-resolve all units and ownership before
    // every strike; never dereference a corpse or a despawned pet pointer.
    void* pet=Curses::Find(game,1,slot.id);void* target=Curses::Find(game,1,slot.target);
    void* owner=pet?Curses::SkeletonOwner(game,pet):nullptr;
    if(!AuraFeatures::Alive(pet)||!Curses::Player(owner)||!AuraFeatures::Alive(owner)||
       static_cast<unsigned*>(pet)[1]!=MonsterClass||!Hostile(game,target)||
       AuraBuffs::Level(game,owner,AuraBuffs::Index(110))<1||
       SoulHarvest::Room(pet)!=SoulHarvest::Room(target)||!InMelee(pet,target)) {
        slot.remaining=0;return;
    }
    if(!Cast(pet,CombatSpells[3][0],slot.level,target)){slot.remaining=0;return;}
    if(Ramp::Log&&ZealLogs++<25){char text[144]{};std::snprintf(text,sizeof(text),"Guardian Zeal strike dispatched: pet=%u target=%u remaining=%d level=%d",slot.id,slot.target,slot.remaining,slot.level);Ramp::Log(text);}
    --slot.remaining;slot.hitAt=now+5; // Five frames between actual strikes.
}
inline void Tick(void* game) {
    if(!game||!Curses::Cast||!Curses::X||!Curses::Y||!SoulHarvest::Room)return;
    const unsigned now=Ramp::Frame(game);
    if(Session!=game||now<LastFrame){Casters={};NextScan=0;Session=game;}
    LastFrame=now;
    for(auto& slot:Casters)if(slot.remaining>0)Burst(game,slot,now);
    if(now<NextScan)return;
    NextScan=now+ScanInterval;
    std::array<void*,64> pets{};std::array<void*,4096> enemies{};unsigned np=0,ne=0;
    AuraFeatures::Units(game,1,[&](void* u){
        if(!AuraFeatures::Alive(u))return;
        if(static_cast<unsigned*>(u)[1]==MonsterClass){if(np<pets.size())pets[np++]=u;}
        else if(ne<enemies.size()&&Hostile(game,u))enemies[ne++]=u;
    });
    for(unsigned i=0;i<np;++i) {
        void* pet=pets[i];Appearance(pet);Caster* slot=Track(pet,now);
        void* owner=Curses::SkeletonOwner(game,pet);
        if(!Curses::Player(owner)||!AuraFeatures::Alive(owner))continue;
        const int level=AuraBuffs::Level(game,owner,AuraBuffs::Index(110));
        if(level<1)continue;
        if(!slot||now<slot->next)continue;
        void* room=SoulHarvest::Room(pet);
        if(!room||SoulHarvest::Room(owner)!=room||Distance(owner,pet)>40*40)continue;
        void* target=nullptr;std::int64_t distance=CastRange*CastRange+1;
        for(unsigned n=0;n<ne;++n) {
            if(SoulHarvest::Room(enemies[n])!=room)continue;
            const auto d=Distance(pet,enemies[n]);
            if(d<distance){target=enemies[n];distance=d;}
        }
        if(!target)continue;
        const int choice=Select(Ramp::GetUnit(owner,12,0),distance,slot->sequence,InMelee(pet,target));
        if(choice<0)continue;
        if(choice==3) {
            slot->target=static_cast<unsigned*>(target)[2];slot->level=std::clamp(level,1,1000);
            slot->remaining=ZealHits(level);ZealAnimation(game,pet,level,slot->remaining);slot->hitAt=now;slot->next=now+25;++slot->sequence;
            Burst(game,*slot,now);continue;
        }
        const int skill=CombatSpells[choice][0];
        // This is the native triggered-skill dispatcher: no unsupported monster
        // casting animation, player input, aura selection or reserved mana change.
        if(Cast(pet,skill,std::clamp(level,1,1000),target)) {
            ++slot->sequence;slot->next=now+(choice==2?35:20);
            if(Ramp::Log&&Logs++<12) {
                char text[160]{};std::snprintf(text,sizeof(text),"Astral Guardian combat: pet=%u owner=%u spell=%d level=%d target=%u",static_cast<unsigned*>(pet)[2],static_cast<unsigned*>(owner)[2],skill,level,static_cast<unsigned*>(target)[2]);Ramp::Log(text);
            }
        } else slot->next=now+25;
    }
}
} // namespace GuardianCombat
