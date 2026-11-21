#pragma once
#include "src/soul_harvest.h"
#include <D2RLPlugin/api.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include "item_auras.h"

namespace AuraBuffs {
#include "aura_bindings.h"
#include "aura_features_bindings.h"
#include "ground_bindings.h"
constexpr unsigned Count=std::size(Definitions);
using SkillFn=int(__fastcall*)(void*,void*,int,int);
using ItemFn=void(__fastcall*)(void*,void*,int,int,int);
inline SkillFn OriginalBuff=nullptr;
inline int (*CustomCast)(void*,void*,int,int)=nullptr;
inline ItemFn OriginalItem=nullptr;
inline std::array<SkillFn,Count> Native{};
inline thread_local bool ItemCall=false;
inline void* Session=nullptr;
inline unsigned SessionFrame=0,Serial=0;
struct Player {
    unsigned id=0xffffffffU;
    std::array<int,Count> costs{},levels{};
    std::array<unsigned,Count> order{},next{};
    unsigned refresh=0;
};
inline std::array<Player,16> Players{};
inline int Index(int skill) {
    for(unsigned i=0;i<Count;++i)if(Definitions[i].skill==skill)return static_cast<int>(i);
    return -1;
}
inline bool PassiveOrSummon(int skill){return skill==100||skill==105||skill==110;}
inline void (*FeatureFrame)(void*) = nullptr;
inline void (*AfterNativePulse)(void*,void*,unsigned,int) = nullptr;
inline bool (*FeaturePulse)(void*,void*,unsigned,int) = nullptr;
inline void (*FeatureDisable)(void*,void*,unsigned) = nullptr;
inline bool InstallFeatures(const D2RL::PluginContext* context);
inline int Cost(unsigned index,int level,int efficiency=0) {
    const auto& aura=Definitions[index];
    const int raw=aura.reserveBase+aura.reservePerLevel*std::clamp(level,1,1000);
    // 150/100 applies the new +50% cost without rounding tier coefficients.
    // Feast has its own explicit 15 + 3/level cost, outside the Paladin surcharge.
    const bool sorceressForm=aura.skill==36||aura.skill==39||aura.skill==43;
    const int factor=sorceressForm?75:(aura.skill==221||aura.skill==224||aura.skill==239||aura.skill==8||aura.skill==17||aura.skill==18||aura.skill==28||aura.skill==131||aura.skill==263||aura.skill==278||aura.skill==276||aura.skill==257||aura.skill==277)?100:150;
    return std::max(1,raw*factor/(100+std::clamp(efficiency,0,10000)));
}
inline int Total(const Player& player){int n=0;for(int cost:player.costs)n+=cost;return n;}
inline bool Dead(void* unit){return !unit||Ramp::GetUnit(unit,6,0)<=0;}
inline void Reset(void* game) {
    const unsigned frame=Ramp::Frame(game);
    if(Session!=game||frame<SessionFrame){Players={};Session=game;Serial=0;}
    SessionFrame=frame;
}
inline Player* Track(void* game,void* unit) {
    if(!game||!Curses::Player(unit))return nullptr;
    Reset(game);
    const unsigned id=static_cast<unsigned*>(unit)[2];
    for(auto& slot:Players)if(slot.id==id)return &slot;
    for(auto& slot:Players)if(slot.id==0xffffffffU){slot={};slot.id=id;return &slot;}
    return nullptr;
}
inline int Level(void* game,void* unit,unsigned index) {
    return std::max(0,Curses::EvalCalc4(game,unit,Definitions[index].levelHelper,1,0));
}
inline unsigned Delay(void* game,void* unit,unsigned index,int level) {
    void* row=Curses::Record(Ramp::Version(game),Definitions[index].skill);
    if(!row)return 50;
    const int formula=*reinterpret_cast<int*>(static_cast<unsigned char*>(row)+0x180);
    return static_cast<unsigned>(std::max(5,Curses::Evaluate(Ramp::Version(game),unit,formula,Definitions[index].skill,level)));
}
inline unsigned Next(void* game,void* unit,unsigned index,int level) {
    const unsigned delay=Delay(game,unit,index,level);
    return (Ramp::Frame(game)+delay-1)/delay*delay+1;
}
inline int Reservation(void* game,void* unit) {
    void* list=Ramp::OriginalList(unit,ReservationState);
    return list?std::max(0,-Curses::Read(game,list,9)):0;
}
inline int UnreservedMana(void* game,void* unit) {
    void* list=Ramp::OriginalList(unit,ReservationState);
    if(!list)return Ramp::GetUnit(unit,9,0);
    const int delta=Curses::Read(game,list,9);
    // Native maximum mana clamps at zero. Adding the reservation to a clamped
    // result cannot recover the real budget after equipment or buffs are lost.
    // Query without this one temporary delta on the server thread, then restore
    // it before returning; no client update or base-stat write occurs here.
    Curses::Write(game,list,9,0);
    const int maximum=Ramp::GetUnit(unit,9,0);
    Curses::Write(game,list,9,delta);
    return maximum;
}
inline void Expire(void* game,void* unit,int state) {
    if(void* list=Ramp::OriginalList(unit,state)) {
        Ramp::SetExpiry(list,static_cast<int>(Ramp::Frame(game)+1));
        Ramp::Schedule(game,unit,12,static_cast<int>(Ramp::Frame(game)+1),0,0,0);
    }
}
inline void SyncReservation(void* game,void* unit,const Player& player) {
    const int amount=Total(player)*256;
    void* list=Ramp::OriginalList(unit,ReservationState);
    if(!amount) {
        if(list){Curses::Write(game,list,9,0);Expire(game,unit,ReservationState);}
    } else {
        if(!list) {
            // A temporary list owns the reservation. Never change the player's
            // saved base mana, and never reserve mana from nearby allies.
            Curses::Curse request{unit,unit,Definitions[0].skill,1,0x1000000,-1,0,ReservationState,nullptr};
            list=Curses::OriginalApply(&request);
        }
        if(!list)return;
        Curses::Write(game,list,9,-amount);
    }
    const int mana=Ramp::GetUnit(unit,8,0),maximum=std::max(0,Ramp::GetUnit(unit,9,0));
    if(mana>maximum)Ramp::SetUnit(unit,8,maximum,0);
    if(SoulHarvest::Refresh)SoulHarvest::Refresh(unit);
    if(SoulHarvest::MarkGfxDirty)SoulHarvest::MarkGfxDirty(unit,ReservationState,amount?1:0);
    Curses::Update(unit,"Aura reservation",0);
}
inline int Pulse(void* game,void* unit,unsigned index,int level) {
    if(FeaturePulse&&FeaturePulse(game,unit,index,level))return 1;
    const int result=Native[index](game,unit,Definitions[index].skill,level);
    if(AfterNativePulse)AfterNativePulse(game,unit,index,level);
    return result;
}
inline void Disable(void* game,void* unit,Player& player,unsigned index) {
    if(FeatureDisable)FeatureDisable(game,unit,index);
    player.costs[index]=0;player.next[index]=0;
    // Equipped aura copies keep their native state and free item lifecycle.
    if(!Ramp::GetUnit(unit,151,Definitions[index].skill))Expire(game,unit,Definitions[index].state);
}
inline void Refresh(void* game,void* unit,Player& player) {
    const int before=Total(player);
    if(Dead(unit)) {
        for(unsigned i=0;i<Count;++i)if(player.costs[i])Disable(game,unit,player,i);
    } else {
        for(unsigned i=0;i<Count;++i)if(player.costs[i]) {
            const int level=Level(game,unit,i);
            if(!level)Disable(game,unit,player,i);
            else{player.levels[i]=level;player.costs[i]=Cost(i,level,Ramp::GetUnit(unit,EfficiencyStat,0));}
        }
        const int maximum=std::max(0,UnreservedMana(game,unit))/256;
        while(Total(player)>maximum) {
            int newest=-1;
            for(unsigned i=0;i<Count;++i)if(player.costs[i]&&(newest<0||player.order[i]>player.order[newest]))newest=static_cast<int>(i);
            if(newest<0)break;
            Disable(game,unit,player,newest);
        }
    }
    if(Total(player)!=before||Reservation(game,unit)!=Total(player)*256)SyncReservation(game,unit,player);
}
inline int __fastcall Toggle(void* game,void* unit,int skill,int level) {
    if(CustomCast){int result=CustomCast(game,unit,skill,level);if(result>=0)return result;}
    const int index=Index(skill);
    if(index<0)return OriginalBuff(game,unit,skill,level);
    if(PassiveOrSummon(skill))return 0;
    if(!Curses::Player(unit)||ItemCall)return Native[index](game,unit,skill,level);
    Player* player=Track(game,unit);
    if(!player)return 0;
    Refresh(game,unit,*player);
    if(player->costs[index]) {
        Disable(game,unit,*player,index);SyncReservation(game,unit,*player);return 1;
    }
    level=Level(game,unit,index);
    const int cost=Cost(index,level,Ramp::GetUnit(unit,EfficiencyStat,0));
    if(Dead(unit)||level<1||cost>Ramp::GetUnit(unit,9,0)/256)return 0;
    // Reserve first, so native healing/damage pulses see the usable maximum.
    player->costs[index]=cost;player->levels[index]=level;player->order[index]=++Serial;
    SyncReservation(game,unit,*player);
    if(!Ramp::OriginalList(unit,ReservationState)||!Pulse(game,unit,index,level)) {
        Disable(game,unit,*player,index);SyncReservation(game,unit,*player);return 0;
    }
    player->next[index]=Next(game,unit,index,level);
    return 1;
}
inline void __fastcall ItemPeriodic(void* game,void* unit,int key,int skill,int extra) {
    const int index=Index(skill);
    const bool previous=ItemCall;
    // Equipment still refers to original skill IDs. Route the five replaced
    // auras to untouched hidden copies rather than passives or a summon.
    for(const auto& legacy:LegacySkills)if(skill==legacy[0]) {
        const int level=Ramp::GetUnit(unit,151,skill);
        if(level>0&&!Dead(unit)) {
            Native[index](game,unit,legacy[1],level);
            Ramp::Schedule(game,unit,9,static_cast<int>(Ramp::Frame(game)+50),key,skill,extra);
        }
        return;
    }
    ItemCall=true;OriginalItem(game,unit,key,skill,extra);ItemCall=previous;
    // Converted records have no periodic flags. Preserve free item/mercenary
    // aura renewal explicitly instead of turning them into player reservations.
    if(index>=0&&!Dead(unit)) {
        const int level=Ramp::GetUnit(unit,151,skill);
        if(level>0)Ramp::Schedule(game,unit,9,static_cast<int>(Next(game,unit,index,level)),key,skill,extra);
    }
}
inline void OnFrame(void* game) {
    if(!game)return;
    Reset(game);
    const unsigned frame=Ramp::Frame(game);
    for(auto& player:Players) {
        if(player.id==0xffffffffU)continue;
        void* unit=Curses::Find(game,0,player.id);
        if(!unit){player={};continue;}
        if(frame>=player.refresh||Dead(unit)) {
            Refresh(game,unit,player);player.refresh=frame+5;
        }
        for(unsigned i=0;i<Count;++i)if(player.costs[i]&&frame>=player.next[i]) {
            Pulse(game,unit,i,player.levels[i]);
            player.next[i]=Next(game,unit,i,player.levels[i]);
        }
    }
}
inline void __fastcall Frame(void* game){OnFrame(game);if(FeatureFrame)FeatureFrame(game);SoulHarvest::OnFrame(game);}

inline bool Install(const D2RL::PluginContext* context) {
    auto exe=reinterpret_cast<unsigned char*>(context->exeBase);
    const unsigned char buff[]={0x48,0x89,0x5c,0x24,0x10,0x48,0x89,0x6c,0x24,0x18,0x48,0x89,0x74,0x24,0x20};
    const unsigned char item[]={0x48,0x89,0x5c,0x24,0x20,0x55,0x56,0x57,0x41,0x54,0x41,0x56,0x48,0x83,0xec,0x40};
    if(!context->api->checkExpectedBytes(context,0x553e60,buff,sizeof(buff))||
       !context->api->checkExpectedBytes(context,0x437230,item,sizeof(item)))return false;
    auto table=reinterpret_cast<SkillFn*>(exe+0x238ea00);
    if(table[18]!=reinterpret_cast<SkillFn>(exe+0x553e60))return false;
    for(unsigned i=0;i<Count;++i)Native[i]=table[Definitions[i].function];
    auto buffHook=D2RL::MakeInlineHook(0x553e60,buff,sizeof(buff),reinterpret_cast<void*>(Toggle),reinterpret_cast<void**>(&OriginalBuff));
    auto itemHook=D2RL::MakeInlineHook(0x437230,item,sizeof(item),reinterpret_cast<void*>(ItemPeriodic),reinterpret_cast<void**>(&OriginalItem));
    if(!context->api->installInlineHook(context,&buffHook)||!context->api->installInlineHook(context,&itemHook))return false;
    SoulHarvest::UnreservedMana=UnreservedMana;
    if(!InstallFeatures(context)||!ItemAuras::Install(context))return false;
    context->LogInfo("17 toggle aura buffs and 3 replacement skills enabled; reservation follows required-level tiers 1/6/12/18/24/30; native item aura renewal retained");
    return true;
}
} // namespace AuraBuffs

#include "aura_features.h"
