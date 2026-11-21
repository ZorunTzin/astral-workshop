#pragma once
// Observe the native successful Redemption result. No extra corpse scan or
// chance roll is introduced, so each actually consumed corpse earns one stack.
namespace RedemptionLife {
#include "redemption_bindings.h"
constexpr unsigned Duration=15*25;
struct Bucket {unsigned until=0;int amount=0;};
struct Player {unsigned id=0xffffffffU,checked=0;int total=0;std::array<Bucket,Duration> buckets{};};
inline std::array<Player,16> Players{};
inline void* Session=nullptr;inline unsigned Last=0;
using RedeemFn=int(__fastcall*)(void*,void*,void*,int,int,int);
inline RedeemFn Original=nullptr;
inline void Reset(void* game){unsigned now=Ramp::Frame(game);if(Session!=game||now<Last){Players={};Session=game;}Last=now;}
inline void Publish(void* game,void* unit,Player& p){
    void* list=Ramp::OriginalList(unit,RedemptionLifeState);
    if(!list&&p.total>0){Curses::Curse request{unit,unit,124,1,static_cast<int>(Duration),-1,0,RedemptionLifeState,nullptr};list=Curses::OriginalApply(&request);}
    if(!list)return;
    // Use the native max-life percentage, independent of Soul Harvest's flat
    // resource deltas. Never bake temporary life into saved base stats.
    Curses::Write(game,list,76,p.total);
    unsigned until=Ramp::Frame(game)+1;
    for(const auto& b:p.buckets)if(b.amount&&b.until>until)until=b.until;
    Ramp::SetExpiry(list,static_cast<int>(until));Ramp::Schedule(game,unit,12,static_cast<int>(until),0,0,0);
    if(SoulHarvest::Refresh)SoulHarvest::Refresh(unit);
    if(SoulHarvest::MarkGfxDirty)SoulHarvest::MarkGfxDirty(unit,RedemptionLifeState,p.total>0);
    Curses::Update(unit,"Redemption maximum life",0);
}
inline void Add(void* game,void* unit,int amount){
    if(amount<1)return;Reset(game);Player* slot=nullptr;unsigned id=static_cast<unsigned*>(unit)[2],now=Ramp::Frame(game);
    for(auto& p:Players)if(p.id==id){slot=&p;break;}
    if(!slot)for(auto& p:Players)if(p.id==0xffffffffU){slot=&p;p.id=id;p.checked=now;break;}
    if(!slot)return;
    auto& bucket=slot->buckets[now%Duration];
    if(bucket.until!=now+Duration){slot->total-=bucket.amount;bucket={now+Duration,0};}
    amount=std::min(amount,10000-slot->total);bucket.amount+=amount;slot->total+=amount;Publish(game,unit,*slot);
}
inline int __fastcall Redeem(void* game,void* owner,void* corpse,int skill,int level,int skip){
    const int result=Original(game,owner,corpse,skill,level,skip);
    if(result&&skill==124&&Curses::Player(owner)&&AuraFeatures::ActiveLevel(game,owner,124)>0)
        Add(game,owner,AuraFeatures::Scale(1+std::clamp(level,1,1000)/10,owner));
    return result;
}
inline void Tick(void* game){
    Reset(game);unsigned now=Ramp::Frame(game);
    for(auto& p:Players){if(p.id==0xffffffffU)continue;void* unit=Curses::Find(game,0,p.id);
        if(!AuraFeatures::Alive(unit)){p={};continue;}
        int old=p.total;
        for(unsigned elapsed=1,limit=std::min(now-p.checked,Duration);elapsed<=limit;++elapsed){auto& b=p.buckets[(p.checked+elapsed)%Duration];if(b.amount&&b.until<=now){p.total-=b.amount;b={};}}
        p.checked=now;if(p.total!=old)Publish(game,unit,p);
    }
}
inline bool Install(const D2RL::PluginContext* context){
    // Build 93847: six-argument native successful-corpse-redemption handler.
    const unsigned char expected[]={0x48,0x89,0x5c,0x24,0x10,0x48,0x89,0x6c,0x24,0x18,0x56,0x41,0x54,0x41,0x55};
    auto hook=D2RL::MakeInlineHook(0x560d90,expected,sizeof(expected),reinterpret_cast<void*>(Redeem),reinterpret_cast<void**>(&Original));
    return context->api->installInlineHook(context,&hook);
}
}
